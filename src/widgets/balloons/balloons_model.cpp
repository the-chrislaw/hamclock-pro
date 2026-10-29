#include "balloons_model.hpp"
#include "../../domain/geometry.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// SondeHub's open API has no stated limit; one request per 10 minutes, cached.
constexpr PollPolicy kPolicy{10min, 1h, 1h};
constexpr char kUrl[] = "https://api.v2.sondehub.org/sondes/telemetry?duration=1h";
constexpr int kMaxAgeS = 30 * 60;
constexpr int kLaterRows = 10;  // a tall pane lists as many as fit; regular layouts show three
constexpr int kMaxMarkers = 300;

QString compass(double deg) {
    static const char* names[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    return names[static_cast<int>((deg + 22.5) / 45.0) % 8];
}
QString km_text(double km) { return QString::number(km, 'f', 0) + " km"; }
} // namespace

BalloonsModel::BalloonsModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("balloons", "Balloons", parent), settings_(context.settings),
      source_(new PolledSource("sondehub-telemetry", QUrl(kUrl), kPolicy, context.transport, this)) {
    connect(source_, &PolledSource::updated, this, [this] {
        feed_ = balloons::parse(source_->body());
        emit balloonsChanged();
        refreshState();
        updateMapLayer();
    });
    connect(source_, &PolledSource::failed, this, &BalloonsModel::refreshState);
    connect(this, &WidgetModel::ageChanged, this, [this] { emit balloonsChanged(); refreshState(); updateMapLayer(); });
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, [this] { emit balloonsChanged(); });
}

void BalloonsModel::start() { source_->start(); }

bool BalloonsModel::hasStation() const { return settings_ && settings_->configured(); }

std::vector<balloons::Near> BalloonsModel::near(int limit) const {
    if (!feed_.ok || !hasStation()) return {};
    return balloons::nearest(feed_, settings_->deLatitude(), settings_->deLongitude(), now(), kMaxAgeS, limit);
}

int BalloonsModel::count() const {
    int n = 0;
    for (const auto& s : feed_.sondes)
        if (s.time.secsTo(now()) <= kMaxAgeS) ++n;
    return n;
}

QString BalloonsModel::nearest() const {
    const auto n = near(1);
    return n.empty() ? QString() : km_text(n.front().distance_km) + " " + compass(n.front().bearing_deg);
}

QString BalloonsModel::nearestDetail() const {
    const auto n = near(1);
    if (n.empty()) return {};
    const auto& s = n.front().sonde;
    QStringList parts{(s.type + " " + s.serial).trimmed(), QString::number(s.alt_m / 1000.0, 'f', 1) + " km up"};
    const auto ph = balloons::phase(s);
    if (!ph.isEmpty()) parts << ph;
    const int eta = balloons::landing_eta_remaining_s(s, s.time.secsTo(now()));
    if (eta == 0) parts << QString("probably landed");
    else if (eta > 0) parts << (eta < 90 ? QString("landing in ~1 min") : QString("landing in ~%1 min").arg((eta + 30) / 60));
    double llat = 0, llon = 0;
    if (balloons::landing_point(s, s.time.secsTo(now()), llat, llon)) {
        const auto g = geometry({settings_->deLatitude(), settings_->deLongitude()}, {llat, llon});
        parts << QString("lands ~%1 %2 from you").arg(km_text(g.distance_km), compass(g.short_bearing_deg));
    }
    if (s.freq_mhz > 0) parts << QString::number(s.freq_mhz, 'f', 3) + " MHz";
    return parts.join(" · ");
}

QVariantList BalloonsModel::later() const {
    QVariantList out;
    const auto n = near(1 + kLaterRows);
    for (size_t i = 1; i < n.size(); ++i)
        out.push_back(QVariantMap{{"dist", km_text(n[i].distance_km) + " " + compass(n[i].bearing_deg)},
                                    {"text", (n[i].sonde.type + " " + n[i].sonde.serial).trimmed()}});
    return out;
}

// One marker per recently heard sonde; removed when the data is stale.
void BalloonsModel::updateMapLayer() {
    OverlayLayer layer;
    layer.id = "balloons";
    layer.labels = OverlayLabelMode::Call;
    layer.min_label_distance_px = 60;
    if (feed_.ok && !source_->isStale(now())) {
        for (const auto& s : feed_.sondes) {
            if (s.time.secsTo(now()) > kMaxAgeS) continue;
            if (static_cast<int>(layer.markers.size()) >= kMaxMarkers) break;
            OverlayMarker m;
            m.where = {s.lat, s.lon};
            m.call = s.serial;
            m.detail = (s.type + " " + s.serial + " · " + QString::number(s.alt_m / 1000.0, 'f', 1) + " km up").trimmed();
            m.color = QColor("#5ec2b7");
            m.shape = OverlayShape::Triangle;
            m.size_px = 4;
            layer.markers.push_back(m);
        }
    }
    if (layer.markers.empty()) MapOverlays::instance().remove_layer("balloons");
    else MapOverlays::instance().set_layer(layer);

    // Where the sonde nearest to DE has been: dots from its older frames.
    // The overlay API has no polylines yet, so the track is a dotted trail.
    OverlayLayer trail;
    trail.id = "balloon-trail";
    trail.labels = OverlayLabelMode::None;
    if (feed_.ok && !source_->isStale(now())) {
        const auto n = near(1);
        if (!n.empty())
            for (const auto& p : n.front().sonde.trail) {
                OverlayMarker m;
                m.where = {p.first, p.second};
                m.color = QColor("#5ec2b7");
                m.size_px = 1.5;
                trail.markers.push_back(m);
            }
    }
    if (trail.markers.empty()) MapOverlays::instance().remove_layer("balloon-trail");
    else MapOverlays::instance().set_layer(trail);
}

void BalloonsModel::refreshState() {
    if (!feed_.ok) {
        if (!source_->lastError().isEmpty()) setState("offline", "SondeHub unreachable: " + source_->lastError());
        else if (source_->fetchedAt().isValid()) setState("stale", "SondeHub returned no usable telemetry");
        else setState("loading", "Contacting SondeHub…");
        return;
    }
    const auto fetched = source_->fetchedAt();
    if (source_->isStale(now())) setState("stale", "SondeHub (out of date)", fetched);
    else setState("live", "SondeHub radiosonde telemetry", fetched);
}

void register_widget_balloons(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new BalloonsModel(context);
    registry.add({"balloons", "Balloons", QUrl("qrc:/widgets/balloons/BalloonsCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &BalloonsModel::start);
}

} // namespace hamclock
