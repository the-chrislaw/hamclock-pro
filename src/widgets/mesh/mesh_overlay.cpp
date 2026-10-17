#include "mesh_overlay.hpp"
#include <QtGlobal>

namespace hamclock::mesh {

OverlayLayer overlay_layer(const std::vector<Node>& nodes, int max_markers) {
    OverlayLayer layer;
    layer.id = kOverlayLayerId;
    layer.labels = OverlayLabelMode::Call;
    layer.min_label_distance_px = 48;
    for (const auto& n : nodes) {
        if (int(layer.markers.size()) >= max_markers) break;
        if (!qIsFinite(n.latitude) || !qIsFinite(n.longitude)) continue;
        OverlayMarker m;
        m.where = {n.latitude, n.longitude};
        m.call = n.short_name.isEmpty() ? n.id : n.short_name;
        m.color = QColor("#8e6bd1");
        m.shape = OverlayShape::Triangle;
        m.size_px = 4;
        m.detail = n.long_name.isEmpty() ? m.call : m.call + " · " + n.long_name;
        layer.markers.push_back(m);
    }
    return layer;
}

} // namespace hamclock::mesh
