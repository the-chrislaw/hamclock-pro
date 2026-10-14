#include "fires_model.hpp"
#include "fires_overlay.hpp"
#include "../../map/overlay.hpp"
#include "../../domain/geometry.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include <cmath>
#include <QLocale>
#include <QTimer>
#include <algorithm>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// FIRMS NRT files update several times a day; 30 minutes is gentle and timely.
constexpr PollPolicy kPolicy{30min, 3h, 6h};
constexpr char kUrl[] = "https://firms.modaps.eosdis.nasa.gov/data/active_fire/noaa-20-viirs-c2/csv/J1_VIIRS_C2_USA_contiguous_and_Hawaii_24h.csv";
} // namespace

FiresModel::FiresModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("fires", "Active fires", parent),
      settings_(context.settings),
      source_(new PolledSource("firms-viirs-usa", QUrl(kUrl), kPolicy, context.transport, this)) {
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, &FiresModel::firesChanged);
    connect(source_, &PolledSource::updated, this, [this] {
        feed_ = fires::parse(source_->body());
        emit firesChanged();
        refreshState();
    });
    connect(source_, &PolledSource::failed, this, &FiresModel::refreshState);
    connect(this, &WidgetModel::ageChanged, this, &FiresModel::refreshState);
}

FiresModel::~FiresModel() { MapOverlays::instance().remove_layer(fires::kOverlayLayerId); }

void FiresModel::start() { source_->start(); }

int FiresModel::highCount() const {
    return static_cast<int>(std::count_if(feed_.detections.begin(), feed_.detections.end(),
                                          [](const fires::Detection& d) { return d.confidence == fires::Detection::High; }));
}

QString FiresModel::summary() const {
    return feed_.ok ? QLocale::c().toString(count()).replace(',', ' ') + (count() == 1 ? " detection" : " detections") : QString();
}

QString FiresModel::detail() const {
    if (feed_.detections.empty()) return {};
    const auto& top = feed_.detections.front();
    QDateTime newest = top.acquired;
    for (const auto& d : feed_.detections) newest = std::max(newest, d.acquired);
    return "strongest " + QString::number(qRound(top.frp_mw)) + " MW at " +
           QString::number(std::abs(top.latitude), 'f', 1) + (top.latitude < 0 ? "S " : "N ") +
           QString::number(std::abs(top.longitude), 'f', 1) + (top.longitude < 0 ? "W" : "E") +
           " · newest " + newest.toString("dd HH:mm") + "Z";
}

QString FiresModel::nearestText() const {
    if (!settings_ || !settings_->configured()) return {};
    const Location de{settings_->deLatitude(), settings_->deLongitude()};
    if (!valid_location(de)) return {};
    const fires::Detection* best = nullptr;
    GeometryResult best_g{};
    for (const auto& d : feed_.detections) {
        if (d.confidence == fires::Detection::Low) continue;
        const auto g = geometry(de, {d.latitude, d.longitude});
        if (!best || g.distance_km < best_g.distance_km) { best = &d; best_g = g; }
    }
    if (!best) return {};
    static const char* const kPoints[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    QString where = QString::number(std::lround(best_g.distance_km)) + " km";
    if (best_g.status == BearingStatus::Defined)
        where += QString(" ") + kPoints[static_cast<int>(std::floor(best_g.short_bearing_deg / 45.0 + 0.5)) % 8] + " of DE";
    return "Nearest " + where + " · " + best->acquired.toString("dd HH:mm") + "Z";
}

void FiresModel::setMapVisible(bool visible) {
    map_visible_ = visible;
    publishOverlay();
}

// Never keep markers for data that is not live.
void FiresModel::publishOverlay() {
    if (map_visible_ && state() == "live") MapOverlays::instance().set_layer(fires::overlay_layer(feed_.detections));
    else MapOverlays::instance().remove_layer(fires::kOverlayLayerId);
}

void FiresModel::refreshState() {
    refreshStateImpl();
    publishOverlay();
}

void FiresModel::refreshStateImpl() {
    if (!feed_.ok) {
        if (!source_->lastError().isEmpty()) setState("offline", "NASA FIRMS unreachable: " + source_->lastError());
        else if (source_->fetchedAt().isValid()) setState("stale", "NASA FIRMS returned no usable fire data");
        else setState("loading", "Contacting NASA FIRMS…");
        return;
    }
    const auto fetched = source_->fetchedAt();
    if (source_->isStale(now())) setState("stale", "NASA FIRMS (fire list out of date)", fetched);
    else setState("live", "NASA FIRMS VIIRS, US + Hawaii, last 24 h", fetched);
}

void register_widget_fires(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new FiresModel(context);
    registry.add({"fires", "Active fires", QUrl("qrc:/widgets/fires/FiresCard.qml"), "strip", 62}, model);
    QTimer::singleShot(0, model, &FiresModel::start);
}

} // namespace hamclock
