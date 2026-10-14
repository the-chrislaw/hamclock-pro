#include "storms_overlay.hpp"
#include <algorithm>
#include <cmath>

namespace hamclock::storms {

OverlayLayer overlay_layer(const std::vector<Storm>& storms) {
    OverlayLayer layer;
    layer.id = kOverlayLayerId;
    layer.labels = OverlayLabelMode::Prefix;
    layer.min_label_distance_px = 36;
    for (const auto& s : storms) {
        if (std::abs(s.latitude) > 90 || std::abs(s.longitude) > 180) continue;
        const int cat = category(s.wind_kt);
        OverlayMarker m;
        m.where = {s.latitude, s.longitude};
        m.call = m.prefix = s.name;
        m.color = QColor(cat >= 3 ? "#d1332b" : cat >= 1 ? "#e08a1e" : "#e5c32b");
        m.shape = OverlayShape::Diamond;
        m.size_px = 8 + 1.5 * cat;
        layer.markers.push_back(m);
    }
    return layer;
}

} // namespace hamclock::storms
