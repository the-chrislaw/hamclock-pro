#include "fires_overlay.hpp"

namespace hamclock::fires {

OverlayLayer overlay_layer(const std::vector<Detection>& detections) {
    OverlayLayer layer;
    layer.id = kOverlayLayerId;
    layer.labels = OverlayLabelMode::None;
    for (const auto& d : detections) {
        if (layer.markers.size() >= kMaxMarkers) break;
        if (d.confidence == Detection::Low) continue;
        OverlayMarker m;
        m.where = {d.latitude, d.longitude};
        m.color = QColor(d.frp_mw >= 50 ? "#d1332b" : "#e08a1e");
        m.shape = OverlayShape::Triangle;
        m.size_px = d.frp_mw >= 50 ? 7 : 5;
        layer.markers.push_back(m);
    }
    return layer;
}

} // namespace hamclock::fires
