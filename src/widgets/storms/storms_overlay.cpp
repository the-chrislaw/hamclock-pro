#include "storms_overlay.hpp"
#include <algorithm>
#include <cmath>

namespace hamclock::storms {

namespace {
QColor intensity_color(int wind_kt) {
    const int cat = category(wind_kt);
    return QColor(cat >= 3 ? "#d1332b" : cat >= 1 ? "#e08a1e" : "#e5c32b");
}
} // namespace

OverlayLayer overlay_layer(const std::vector<Storm>& storms, const std::map<QString, Forecast>& forecasts) {
    OverlayLayer layer;
    layer.id = kOverlayLayerId;
    layer.labels = OverlayLabelMode::Prefix;
    layer.min_label_distance_px = 36;
    for (const auto& s : storms) {
        if (std::abs(s.latitude) > 90 || std::abs(s.longitude) > 180) continue;
        OverlayMarker m;
        m.where = {s.latitude, s.longitude};
        m.call = m.prefix = s.name;
        m.color = intensity_color(s.wind_kt);
        m.shape = OverlayShape::Diamond;
        m.size_px = 8 + 1.5 * category(s.wind_kt);
        layer.markers.push_back(m);
    }
    // Forecast dots go after every storm marker so a storm is never hidden behind one.
    for (const auto& s : storms) {
        const auto it = forecasts.find(s.id);
        if (it == forecasts.end()) continue;
        for (const auto& p : it->second.points) {
            OverlayMarker m;
            m.where = {p.latitude, p.longitude};
            m.color = intensity_color(p.wind_kt);
            m.size_px = p.outlook ? 3 : 4;
            m.detail = QString("%1 %2 %3Z · %4 kt").arg(s.name, p.outlook ? "outlook" : "forecast", p.valid.toString("dd HH:mm"), QString::number(p.wind_kt));
            layer.markers.push_back(m);
        }
    }
    return layer;
}

} // namespace hamclock::storms
