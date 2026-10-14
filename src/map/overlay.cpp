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

namespace hamclock {
std::optional<QRectF> place_label(QRectF view, QPointF anchor, double marker_r, QSizeF text, const std::vector<QRectF>& used) {
    const double gap = 3;
    const double top = std::clamp(anchor.y() - text.height() / 2, view.top(), std::max(view.top(), view.bottom() - text.height()));
    for (const double left : {anchor.x() + marker_r + gap, anchor.x() - marker_r - gap - text.width()}) {
        const QRectF r(left, top, text.width(), text.height());
        if (r.left() < view.left() || r.right() > view.right() || r.top() < view.top() || r.bottom() > view.bottom()) continue;
        bool clash = false;
        for (const auto& u : used) clash = clash || u.intersects(r);
        if (!clash) return r;
    }
    return std::nullopt;
}

MarkerHit hit_test_marker(const std::vector<OverlayLayer>& layers, const MapFrame& frame, double px, double py, double radius_px) {
    MarkerHit best;
    double best_d2 = radius_px * radius_px;
    for (const auto& layer : layers) {
        if (!layer.visible) continue;
        for (const auto& m : layer.markers) {
            const auto p = frame.to_pixel(m.where);
            if (!p) continue;
            const double dx = p->x - px, dy = p->y - py, d2 = dx * dx + dy * dy;
            if (d2 <= best_d2) { best_d2 = d2; best = {&layer, &m}; }
        }
    }
    return best;
}
MarkerHit hit_test_marker(const std::vector<OverlayLayer>& layers, const MapViewport& vp, double px, double py, double radius_px) {
    MarkerHit best;
    double best_d2 = radius_px * radius_px;
    for (const auto& layer : layers) {
        if (!layer.visible) continue;
        for (const auto& m : layer.markers) {
            const double x = vp.x_of(m.where.longitude), y = vp.y_of(m.where.latitude);
            for (int k = -1; k <= 1; ++k) {
                const double dx = x + k * vp.world_width() - px, dy = y - py, d2 = dx * dx + dy * dy;
                if (d2 <= best_d2) { best_d2 = d2; best = {&layer, &m}; }
            }
        }
    }
    return best;
}
}
