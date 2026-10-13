#include "overlay.hpp"
#include <algorithm>
#include <cmath>
namespace hamclock {
QString overlay_label_text(const OverlayMarker& m, OverlayLabelMode mode) {
    switch (mode) {
    case OverlayLabelMode::Prefix: return m.prefix.isEmpty() ? m.call : m.prefix;
    case OverlayLabelMode::Call: return m.call;
    default: return {};
    }
}
std::vector<bool> select_labels(const std::vector<QPointF>& anchors, double min_distance_px) {
    std::vector<bool> keep(anchors.size(), false);
    std::vector<QPointF> shown;
    for (std::size_t i = 0; i < anchors.size(); ++i) {
        const bool clear = std::none_of(shown.begin(), shown.end(), [&](const QPointF& s) {
            return std::hypot(s.x() - anchors[i].x(), s.y() - anchors[i].y()) < min_distance_px;
        });
        if (clear) { keep[i] = true; shown.push_back(anchors[i]); }
    }
    return keep;
}
MapOverlays& MapOverlays::instance() {
    static MapOverlays registry;
    return registry;
}
void MapOverlays::set_layer(OverlayLayer layer) {
    auto it = std::find_if(layers_.begin(), layers_.end(), [&](const OverlayLayer& l) { return l.id == layer.id; });
    if (it != layers_.end()) *it = std::move(layer); else layers_.push_back(std::move(layer));
    notify();
}
void MapOverlays::remove_layer(const QString& id) {
    const auto before = layers_.size();
    layers_.erase(std::remove_if(layers_.begin(), layers_.end(), [&](const OverlayLayer& l) { return l.id == id; }), layers_.end());
    if (layers_.size() != before) notify();
}
int MapOverlays::on_change(std::function<void()> listener) {
    listeners_.emplace_back(next_token_, std::move(listener));
    return next_token_++;
}
void MapOverlays::remove_listener(int token) {
    listeners_.erase(std::remove_if(listeners_.begin(), listeners_.end(), [&](const auto& l) { return l.first == token; }), listeners_.end());
}
void MapOverlays::notify() const {
    const auto copy = listeners_; // a listener may unregister while notified
    for (const auto& l : copy) l.second();
}
}
