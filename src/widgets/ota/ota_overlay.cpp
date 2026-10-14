#include "ota_overlay.hpp"
#include <cmath>

namespace hamclock::ota {

OverlayLayer overlay_layer(const std::vector<Spot>& spots, int max_markers) {
    OverlayLayer layer;
    layer.id = kOverlayLayerId;
    layer.labels = OverlayLabelMode::Call;
    layer.min_label_distance_px = 44;
    for (const auto& s : spots) {
        if (int(layer.markers.size()) >= max_markers) break;
        if (!s.has_position || std::abs(s.latitude) > 90 || std::abs(s.longitude) > 180) continue;
        OverlayMarker m;
        m.where = {s.latitude, s.longitude};
        m.call = s.call;
        m.prefix = s.reference;
        const bool sota = s.program == "SOTA";
        m.color = QColor(sota ? "#e08a1e" : "#2e9e5b");
        m.shape = sota ? OverlayShape::Triangle : OverlayShape::Dot;
        m.size_px = 5;
        layer.markers.push_back(m);
    }
    return layer;
}

} // namespace hamclock::ota
