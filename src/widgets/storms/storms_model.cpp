#include "storms_model.hpp"
#include "storms_overlay.hpp"
#include "../../map/overlay.hpp"
#include "../../net/polled_source.hpp"
#include <QRegularExpression>
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// NHC refreshes the index with each advisory (every 3-6 h, more often for
// intermediate advisories); 30 minutes is gentle and timely.
constexpr PollPolicy kPolicy{30min, 3h, 6h};
constexpr char kUrl[] = "https://www.nhc.noaa.gov/CurrentStorms.json";
constexpr int kOtherRows = 3;
constexpr size_t kForecastStorms = 3;  // forecast text fetched for the strongest few only

QString position(const storms::Storm& s) {
    return QString::number(std::abs(s.latitude), 'f', 1) + (s.latitude < 0 ? "S " : "N ") +
           QString::number(std::abs(s.longitude), 'f', 1) + (s.longitude < 0 ? "W" : "E");
}
QString storm_title(const storms::Storm& s) { return storms::classification_text(s.classification) + " " + s.name; }
QString intensity_text(const storms::Storm& s) {
    QStringList parts{storms::classification_text(s.classification)};
    if (storms::category(s.wind_kt) > 0) parts.back() += " cat " + QString::number(storms::category(s.wind_kt));
    parts << QString::number(s.wind_kt) + " kt";
    if (s.pressure_mb) parts << QString::number(s.pressure_mb) + " mb";
    return parts.join(" · ");
}
QString detail(const storms::Storm& s) {
    QStringList parts;
    if (s.speed_kt >= 0 && s.heading_deg >= 0) parts << "moving " + storms::compass(s.heading_deg) + " " + QString::number(s.speed_kt) + " kt";
    parts << position(s);
    if (s.issued.isValid()) parts << "issued " + s.issued.toString("dd HH:mm") + "Z";
    return parts.join(" · ");
}
} // namespace

StormsModel::StormsModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("storms", "Tropical storms", parent),
      transport_(context.transport),
      source_(new PolledSource("nhc-current-storms", QUrl(kUrl), kPolicy, context.transport, this)) {
    // Unusable answers (schema change, cut-off body) keep the last good data.
    source_->setValidator([](const QByteArray& body) { return storms::parse(body).ok; });
    connect(source_, &PolledSource::updated, this, [this] {
        feed_ = storms::parse(source_->body());
        syncForecastSources();
        emit stormsChanged();
        refreshState();
    });
    connect(source_, &PolledSource::failed, this, &StormsModel::refreshState);
    connect(this, &WidgetModel::ageChanged, this, &StormsModel::refreshState);
}

void StormsModel::start() { source_->start(); }

QString StormsModel::headline() const { return feed_.storms.empty() ? QString() : feed_.storms.front().name; }
QString StormsModel::headlineIntensity() const { return feed_.storms.empty() ? QString() : intensity_text(feed_.storms.front()); }
QString StormsModel::headlineDetail() const { return feed_.storms.empty() ? QString() : detail(feed_.storms.front()); }
int StormsModel::headlineCategory() const { return feed_.storms.empty() ? 0 : storms::category(feed_.storms.front().wind_kt); }

QVariantList StormsModel::others() const {
    QVariantList out;
    for (size_t i = 1; i < feed_.storms.size() && out.size() < kOtherRows; ++i)
        out.push_back(QVariantMap{{"text", storm_title(feed_.storms[i]) + " · " + QString::number(feed_.storms[i].wind_kt) + " kt"}, {"detail", detail(feed_.storms[i])}});
    return out;
}

StormsModel::~StormsModel() { MapOverlays::instance().remove_layer(storms::kOverlayLayerId); }

// One PolledSource per storm for NHC's forecast/advisory text (same host and
// cadence as the index; the page only changes with an advisory).
void StormsModel::syncForecastSources() {
    QHash<QString, storms::Storm> wanted;
    for (size_t i = 0; i < feed_.storms.size() && i < kForecastStorms; ++i)
        if (!feed_.storms[i].forecast_url.isEmpty()) wanted.insert(feed_.storms[i].id, feed_.storms[i]);
    for (auto it = forecast_sources_.begin(); it != forecast_sources_.end();) {
        const auto w = wanted.constFind(it.key());
        if (w == wanted.constEnd() || w->forecast_url != it->url) {
            it->source->deleteLater();
            it = forecast_sources_.erase(it);
        } else {
            it->advisory = w->forecast_adv;
            it->issued = w->issued;
            ++it;
        }
    }
    for (auto it = wanted.constBegin(); it != wanted.constEnd(); ++it) {
        if (forecast_sources_.contains(it.key())) continue;
        QString cache = it.key().toLower();
        cache.remove(QRegularExpression("[^a-z0-9]"));
        auto* src = new PolledSource("nhc-forecast-" + cache, QUrl(it->forecast_url), kPolicy, transport_, this);
        src->setClock([this] { return now(); });
        src->setAcceptsHtml(true); // the advisory is an .shtml page with the text in <pre>
        connect(src, &PolledSource::updated, this, &StormsModel::refreshState);
        forecast_sources_.insert(it.key(), {src, it->forecast_url, it->forecast_adv, it->issued});
        src->start();
    }
}

// Only forecasts that are fresh and belong to the advisory the index announced:
// otherwise a cached or lagging page could draw a track for an old position.
std::map<QString, storms::Forecast> StormsModel::forecasts() const {
    std::map<QString, storms::Forecast> out;
    for (auto it = forecast_sources_.constBegin(); it != forecast_sources_.constEnd(); ++it) {
        if (!it->source->fetchedAt().isValid() || it->source->isStale(now())) continue;
        auto fc = storms::parse_forecast(it->source->body(), it->issued);
        if (!fc.ok || fc.storm_id.compare(it.key(), Qt::CaseInsensitive) != 0 || fc.advisory != it->advisory.toInt()) continue;
        out.emplace(it.key(), std::move(fc));
    }
    return out;
}

void StormsModel::setMapVisible(bool visible) {
    map_visible_ = visible;
    publishOverlay();
}

// Never keep markers for data that is not live.
void StormsModel::publishOverlay() {
    if (map_visible_ && state() == "live") MapOverlays::instance().set_layer(storms::overlay_layer(feed_.storms, forecasts()));
    else MapOverlays::instance().remove_layer(storms::kOverlayLayerId);
}

void StormsModel::refreshState() {
    refreshStateImpl();
    publishOverlay();
}

void StormsModel::refreshStateImpl() {
    if (!feed_.ok) {
        if (!source_->lastError().isEmpty()) setState("offline", "NHC unreachable: " + source_->lastError());
        else if (source_->fetchedAt().isValid()) setState("stale", "NHC returned no usable storm data");
        else setState("loading", "Contacting NHC…");
        return;
    }
    const auto fetched = source_->fetchedAt();
    if (source_->isStale(now())) setState("stale", "NHC (storm list out of date)", fetched);
    else setState("live", "NHC active storms (Atlantic, E/C Pacific)", fetched);
}

void register_widget_storms(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new StormsModel(context);
    registry.add({"storms", "Tropical storms", QUrl("qrc:/widgets/storms/StormsCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &StormsModel::start);
}

} // namespace hamclock
