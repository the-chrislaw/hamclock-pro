#include "quakes_overlay.hpp"
#include <algorithm>
#include <cmath>

namespace hamclock::quakes {

OverlayLayer overlay_layer(const std::vector<Quake>& quakes, int max_markers) {
    std::vector<const Quake*> order;
    for (const auto& q : quakes)
        if (std::abs(q.latitude) <= 90 && std::abs(q.longitude) <= 180) order.push_back(&q);
    std::stable_sort(order.begin(), order.end(),
                     [](const Quake* a, const Quake* b) { return a->magnitude > b->magnitude; });
    OverlayLayer layer;
    layer.id = kOverlayLayerId;
    layer.labels = OverlayLabelMode::Prefix;
    layer.min_label_distance_px = 36;
    for (const Quake* q : order) {
        if (int(layer.markers.size()) >= max_markers) break;
        OverlayMarker m;
        m.where = {q->latitude, q->longitude};
        m.call = m.prefix = "M" + QString::number(q->magnitude, 'f', 1);
        m.color = QColor(q->magnitude >= 6 ? "#d1332b" : q->magnitude >= 4.5 ? "#e08a1e" : "#e5c32b");
        m.shape = OverlayShape::Dot;
        m.size_px = std::clamp(2.0 + q->magnitude, 3.0, 12.0);
        layer.markers.push_back(m);
    }
    return layer;
}

} // namespace hamclock::quakes
