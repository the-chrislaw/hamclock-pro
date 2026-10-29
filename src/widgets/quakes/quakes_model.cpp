#include "quakes_model.hpp"
#include "quakes_overlay.hpp"
#include "../../map/overlay.hpp"
#include "../../domain/geometry.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include <cmath>
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// USGS rebuilds the feed every minute and asks clients not to poll faster
// than that; every 5 minutes is plenty for a dashboard.
constexpr PollPolicy kPolicy{5min, 30min, 30min};
constexpr char kUrl[] = "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_day.geojson";
constexpr int kRecentRows = 15;  // a tall pane lists as many as fit; regular layouts show six
} // namespace

QuakesModel::QuakesModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("quakes", "Earthquakes", parent),
      settings_(context.settings),
      source_(new PolledSource("usgs-quakes-2-5-day", QUrl(kUrl), kPolicy, context.transport, this)) {
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, &QuakesModel::quakesChanged);
    connect(source_, &PolledSource::updated, this, [this] {
        feed_ = quakes::parse(source_->body());
        emit quakesChanged();
        refreshState();
    });
    connect(source_, &PolledSource::failed, this, &QuakesModel::refreshState);
    connect(this, &WidgetModel::ageChanged, this, &QuakesModel::refreshState);
}

void QuakesModel::start() { source_->start(); }

double QuakesModel::strongestMagnitude() const {
    const auto* q = quakes::strongest(feed_);
    return q ? q->magnitude : 0;
}

QString QuakesModel::strongestText() const {
    const auto* q = quakes::strongest(feed_);
    if (!q) return {};
    return quakes::magnitude_class(q->magnitude) + " · " + formatAge(q->time, now());
}

QString QuakesModel::strongestPlace() const {
    const auto* q = quakes::strongest(feed_);
    return q ? q->place : QString();
}

QString QuakesModel::nearestText() const {
    if (!settings_ || !settings_->configured() || feed_.quakes.empty()) return {};
    const Location de{settings_->deLatitude(), settings_->deLongitude()};
    if (!valid_location(de)) return {};
    const quakes::Quake* best = nullptr;
    GeometryResult best_g{};
    for (const auto& q : feed_.quakes) {
        const auto g = geometry(de, {q.latitude, q.longitude});
        if (!best || g.distance_km < best_g.distance_km) { best = &q; best_g = g; }
    }
    static const char* const kPoints[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    QString where = QString::number(std::lround(best_g.distance_km)) + " km";
    if (best_g.status == BearingStatus::Defined)
        where += QString(" ") + kPoints[static_cast<int>(std::floor(best_g.short_bearing_deg / 45.0 + 0.5)) % 8] + " of DE";
    return "M" + QString::number(best->magnitude, 'f', 1) + " · " + where + " · " + formatAge(best->time, now());
}

QVariantList QuakesModel::recent() const {
    QVariantList out;
    for (const auto& q : feed_.quakes) {
        if (out.size() >= kRecentRows) break;
        out.push_back(QVariantMap{{"mag", QString::number(q.magnitude, 'f', 1)},
                                  {"place", q.place},
                                  {"age", formatAge(q.time, now())},
                                  {"tsunami", q.tsunami}});
    }
    return out;
}

QuakesModel::~QuakesModel() { MapOverlays::instance().remove_layer(quakes::kOverlayLayerId); }

void QuakesModel::setMapVisible(bool visible) {
    map_visible_ = visible;
    publishOverlay();
}

// Never keep markers for data that is not live.
void QuakesModel::publishOverlay() {
    if (map_visible_ && state() == "live") MapOverlays::instance().set_layer(quakes::overlay_layer(feed_.quakes));
    else MapOverlays::instance().remove_layer(quakes::kOverlayLayerId);
}

void QuakesModel::refreshState() {
    refreshStateImpl();
    publishOverlay();
}

void QuakesModel::refreshStateImpl() {
    if (!feed_.ok) {
        if (!source_->lastError().isEmpty()) setState("offline", "USGS unreachable: " + source_->lastError());
        else if (source_->fetchedAt().isValid()) setState("stale", "USGS returned no usable earthquake data");
        else setState("loading", "Contacting USGS…");
        return;
    }
    // Age of the feed's own build time, falling back to the download time.
    const auto built = feed_.generated.isValid() ? feed_.generated : source_->fetchedAt();
    if (source_->isStale(now())) setState("stale", "USGS (feed out of date)", built);
    else setState("live", "USGS M2.5+ past day", built);
}

void register_widget_quakes(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new QuakesModel(context);
    registry.add({"quakes", "Earthquakes", QUrl("qrc:/widgets/quakes/QuakesCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &QuakesModel::start);
}

} // namespace hamclock
