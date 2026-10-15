#include "eme_model.hpp"
#include "../../ui/app_settings.hpp"
#include <QDateTime>
#include <QTimeZone>

namespace hamclock {
namespace {
QString hhmm(std::int64_t t) {
    return QDateTime::fromSecsSinceEpoch(t, QTimeZone::utc()).toString("HH:mm") + "Z";
}
QString azel(UtcSeconds now, double lat, double lon) {
    const MoonHorizontal h = moon_horizontal(now, lat, lon);
    return QString("Az %1° El %2°").arg(qRound(h.azimuth_deg) % 360).arg(qRound(h.elevation_deg));
}
} // namespace

EmeSummary summarize_eme(UtcSeconds now, double de_lat, double de_lon, double dx_lat, double dx_lon) {
    EmeSummary out;
    out.de_position = azel(now, de_lat, de_lon);
    out.dx_position = azel(now, dx_lat, dx_lon);
    const MoonPosition p = moon_position(now);
    out.distance = QString("%1 km").arg(qRound(p.distance_km / 100.0) * 100);
    out.degradation = QString("+%1 dB vs perigee").arg(eme_degradation_db(p.distance_km), 0, 'f', 1);
    out.declination = QString("Dec %1°").arg(p.dec_deg, 0, 'f', 1);
    const double sep = moon_sun_separation_deg(now);
    out.sun = QString("%1° from Moon%2").arg(qRound(sep)).arg(sep < 10.0 ? " (sun noise)" : "");
    if (const auto w = next_mutual_moon_window(now, de_lat, de_lon, dx_lat, dx_lon)) {
        out.mutual_now = w->in_progress;
        const std::int64_t minutes = (w->end.value - now.value + 30) / 60;
        out.window = w->in_progress
            ? QString("Now until %1 (%2 h %3 min)").arg(hhmm(w->end.value)).arg(minutes / 60).arg(minutes % 60, 2, 10, QChar('0'))
            : QString("%1 to %2").arg(hhmm(w->start.value), hhmm(w->end.value));
    } else {
        out.window = "None in 48 h";
    }
    return out;
}

EmeModel::EmeModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("eme", "EME", parent), settings_(context.settings) {
    timer_.setInterval(60 * 1000);
    connect(&timer_, &QTimer::timeout, this, &EmeModel::recompute);
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, &EmeModel::recompute);
}

void EmeModel::start() { recompute(); timer_.start(); }

void EmeModel::recompute() {
    if (!settings_ || !settings_->configured()) {
        has_station_ = false;
        s_ = {};
        setState("unconfigured", "Set your station location in Settings");
        emit changed();
        return;
    }
    s_ = summarize_eme(UtcSeconds{now().toSecsSinceEpoch()}, settings_->deLatitude(), settings_->deLongitude(),
                       settings_->dxLatitude(), settings_->dxLongitude());
    has_station_ = true;
    setState("live", "Computed offline from the system clock", now());
    emit changed();
}

void register_widget_eme(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new EmeModel(context);
    registry.add({"eme", "EME", QUrl("qrc:/widgets/eme/EmeCard.qml"), "strip", 55}, model);
    QTimer::singleShot(0, model, &EmeModel::start);
}

} // namespace hamclock
