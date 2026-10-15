#include "planets_model.hpp"
#include "../../ui/app_settings.hpp"
#include <QVariantMap>
#include <algorithm>
#include <vector>

namespace hamclock {

QVariantList planet_rows(UtcSeconds now, double de_lat, double de_lon) {
    struct Up { Planet p; PlanetHorizontal h; };
    std::vector<Up> up;
    for (int i = 0; i < kPlanetCount; ++i) {
        const auto p = static_cast<Planet>(i);
        const PlanetHorizontal h = planet_horizontal(p, now, de_lat, de_lon);
        if (h.elevation_deg > 0) up.push_back({p, h});
    }
    std::sort(up.begin(), up.end(), [](const Up& a, const Up& b) { return a.h.elevation_deg > b.h.elevation_deg; });
    QVariantList rows;
    for (const Up& u : up)
        rows.append(QVariantMap{{"name", QString::fromLatin1(planet_name(u.p))},
                                {"position", QString("El %1°  Az %2°").arg(qRound(u.h.elevation_deg)).arg(qRound(u.h.azimuth_deg) % 360)}});
    return rows;
}

OverlayLayer planet_overlay_layer(UtcSeconds now) {
    static const char* colors[kPlanetCount] = {"#b0a79a", "#f5e6a8", "#e0603b", "#d9a066", "#cdb87a", "#8fd3d9", "#5b7fe0"};
    OverlayLayer layer;
    layer.id = "planets";
    layer.labels = OverlayLabelMode::Call;
    layer.min_label_distance_px = 30;
    for (int i = 0; i < kPlanetCount; ++i) {
        const auto p = static_cast<Planet>(i);
        OverlayMarker m;
        const PlanetSubpoint s = planet_subpoint(p, now);
        m.where = {s.latitude_deg, s.longitude_deg};
        m.call = QString::fromLatin1(planet_name(p));
        m.detail = QString("%1 is overhead here").arg(m.call);
        m.color = QColor(colors[i]);
        m.shape = OverlayShape::Dot;
        m.size_px = 4;
        layer.markers.push_back(m);
    }
    return layer;
}

PlanetsModel::PlanetsModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("planets", "Planets", parent), settings_(context.settings) {
    timer_.setInterval(60 * 1000);
    connect(&timer_, &QTimer::timeout, this, &PlanetsModel::recompute);
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, &PlanetsModel::recompute);
}

void PlanetsModel::start() { recompute(); timer_.start(); }

void PlanetsModel::recompute() {
    const UtcSeconds t{now().toSecsSinceEpoch()};
    MapOverlays::instance().set_layer(planet_overlay_layer(t));
    has_station_ = settings_ && settings_->configured();
    rows_ = has_station_ ? planet_rows(t, settings_->deLatitude(), settings_->deLongitude()) : QVariantList{};
    if (has_station_) setState("live", "Computed offline from the system clock (JPL approximate elements, about 0.2 degree)", now());
    else setState("unconfigured", "Set your station location in Settings");
    emit changed();
}

void register_widget_planets(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new PlanetsModel(context);
    registry.add({"planets", "Planets", QUrl("qrc:/widgets/planets/PlanetsCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &PlanetsModel::start);
}

} // namespace hamclock
