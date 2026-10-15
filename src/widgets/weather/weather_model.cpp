#include "weather_model.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include <QTimer>
#include <cmath>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// Open-Meteo's free tier allows far more; 15 min matches the model's own
// update interval and keeps two requests per quarter hour.
constexpr PollPolicy kPolicy{15min, 90min, 1h};
// Grid cells are ~11 km; a larger gap means the document is for another place.
constexpr double kMaxOffsetDeg = 0.5;
// Nominatim usage policy: at most 1 request/s, identifying User-Agent, cache
// results. Place names barely change, so refresh weekly; the first fetch of
// each end is delayed (DE 1.2 s, DX 2.4 s) so a burst of DX changes collapses
// into one request and the two ends never fire together.
constexpr PollPolicy kPlacePolicy{std::chrono::hours(24 * 7), std::chrono::hours(24 * 30), 6h};
} // namespace

WeatherModel::WeatherModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("weather", "Weather", parent), context_(context), settings_(context.settings) {
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, [this] { build(); });
    connect(this, &WidgetModel::ageChanged, this, [this] { refreshState(); emit changed(); });
}

bool WeatherModel::hasStation() const { return settings_ && settings_->configured(); }

void WeatherModel::start() { build(); }

void WeatherModel::build() {
    if (hasStation()) {
        rebuildEnd(de_, "openmeteo-de", settings_->deLatitude(), settings_->deLongitude());
        rebuildEnd(dx_, "openmeteo-dx", settings_->dxLatitude(), settings_->dxLongitude());
    }
    refreshState();
    emit changed();
}

void WeatherModel::rebuildEnd(End& end, const QString& cache_name, double lat, double lon) {
    const double rlat = std::round(lat * 100) / 100, rlon = std::round(lon * 100) / 100;
    if (end.source && end.lat == rlat && end.lon == rlon) return;
    delete end.source;
    delete end.place_source;
    end.place_source = nullptr;
    end.place.clear();
    end.now.reset();
    end.lat = rlat;
    end.lon = rlon;
    // Cache name carries the position so a moved station starts clean.
    const auto tag = [](double v) { return QString::number(static_cast<int>(std::lround(v * 100))).replace('-', 'm'); };
    end.source = new PolledSource(cache_name + "-" + tag(rlat) + "-" + tag(rlon),
                                  QUrl(weather::request_url(rlat, rlon)), kPolicy, context_.transport, this);
    End* e = &end;
    connect(end.source, &PolledSource::updated, this, [this, e] { parse(*e); refreshState(); emit changed(); });
    connect(end.source, &PolledSource::failed, this, &WeatherModel::refreshState);
    end.source->start();

    const bool is_de = &end == &de_;
    end.place_source = new PolledSource("nominatim-" + cache_name + "-" + tag(rlat) + "-" + tag(rlon),
                                        QUrl(weather::reverse_url(rlat, rlon)), kPlacePolicy, context_.transport, this);
    connect(end.place_source, &PolledSource::updated, this, [this, e] { parsePlace(*e); emit changed(); });
    PolledSource* ps = end.place_source;
    QTimer::singleShot(is_de ? 1200 : 2400, ps, [ps] { ps->start(); });
}

void WeatherModel::parsePlace(End& end) {
    end.place = weather::parse_place(end.place_source->body());
}

void WeatherModel::parse(End& end) {
    end.now = weather::parse_current(end.source->body());
    if (end.now && (std::abs(end.now->latitude - end.lat) > kMaxOffsetDeg ||
                    std::abs(end.now->longitude - end.lon) > kMaxOffsetDeg))
        end.now.reset();
}

QString WeatherModel::temp(const End& e) {
    return e.now ? QString("%1 °C").arg(e.now->temperature_c, 0, 'f', 1) : QString();
}
QString WeatherModel::summary(const End& e) {
    return e.now ? weather::wmo_description(e.now->weather_code) : QString();
}
QString WeatherModel::wind(const End& e) {
    if (!e.now) return {};
    if (e.now->wind_kmh < 1) return "Calm";
    return QString("%1 %2 km/h").arg(weather::compass_point(e.now->wind_dir_deg)).arg(std::lround(e.now->wind_kmh));
}

QString WeatherModel::local(const End& e) const {
    return e.now && e.now->utc_offset_s ? weather::local_time_text(now(), *e.now->utc_offset_s) : QString();
}

QString WeatherModel::air(const End& e) {
    if (!e.now) return {};
    QStringList parts{QString("%1% RH").arg(e.now->humidity_pct)};
    if (const auto dew = weather::dew_point_c(e.now->temperature_c, e.now->humidity_pct))
        parts << QString("dew %1 °C").arg(std::lround(*dew));
    if (e.now->pressure_hpa) {
        QString p = QString("%1 hPa").arg(std::lround(*e.now->pressure_hpa));
        if (e.now->pressure_change_hpa) p += " " + weather::pressure_trend_word(*e.now->pressure_change_hpa).toLower();
        parts << p;
    }
    if (e.now->cloud_pct) parts << QString("%1% cloud").arg(*e.now->cloud_pct);
    if (e.now->high_c && e.now->low_c)
        parts << QString("H %1 / L %2 °C").arg(std::lround(*e.now->high_c)).arg(std::lround(*e.now->low_c));
    return parts.join(" · ");
}

void WeatherModel::refreshState() {
    if (!hasStation()) { setState("unconfigured", "Set DE and DX locations in Settings"); return; }
    End* ends[] = {&de_, &dx_};
    QStringList stale, missing;
    QDateTime oldest;
    bool any_error = false;
    const auto now_utc = now();
    for (End* e : ends) {
        const QString label = e == &de_ ? "DE" : "DX";
        if (!e->source) continue;
        any_error = any_error || !e->source->lastError().isEmpty();
        if (!e->now) { missing << label; continue; }
        if (e->source->isStale(now_utc)) stale << label;
        if (!oldest.isValid() || e->source->fetchedAt() < oldest) oldest = e->source->fetchedAt();
    }
    if (missing.size() == 2) {
        if (any_error) setState("offline", "Open-Meteo unreachable: " + de_.source->lastError());
        else setState("loading", "Contacting Open-Meteo…");
        return;
    }
    if (!stale.isEmpty() || !missing.isEmpty()) {
        QStringList parts;
        if (!stale.isEmpty()) parts << "out of date: " + stale.join(", ");
        if (!missing.isEmpty()) parts << "missing: " + missing.join(", ");
        setState("stale", "Open-Meteo (" + parts.join("; ") + ")", oldest);
        return;
    }
    setState("live", de_.place.isEmpty() && dx_.place.isEmpty() ? "Weather by Open-Meteo.com"
                                                                : "Weather by Open-Meteo.com · places © OpenStreetMap contributors", oldest);
}

void register_widget_weather(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new WeatherModel(context);
    registry.add({"weather", "Weather", QUrl("qrc:/widgets/weather/WeatherCard.qml"), "strip", 50}, model);
    QTimer::singleShot(0, model, &WeatherModel::start);
}

} // namespace hamclock
