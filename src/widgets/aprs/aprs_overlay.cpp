#include "aprs_overlay.hpp"
#include <QtGlobal>

namespace hamclock::aprs {

OverlayLayer overlay_layer(const std::vector<Station>& stations, int max_markers) {
    OverlayLayer layer;
    layer.id = kOverlayLayerId;
    layer.labels = OverlayLabelMode::Call;
    layer.min_label_distance_px = 48;
    for (const auto& s : stations) {
        if (int(layer.markers.size()) >= max_markers) break;
        if (!qIsFinite(s.latitude) || !qIsFinite(s.longitude)) continue;
        OverlayMarker m;
        m.where = {s.latitude, s.longitude};
        m.call = s.call;
        m.color = QColor("#2a9d8f");
        m.shape = OverlayShape::Diamond;
        m.size_px = 4;
        m.detail = s.comment.isEmpty() ? s.call : s.call + " · " + s.comment;
        layer.markers.push_back(m);
    }
    return layer;
}

} // namespace hamclock::aprs
