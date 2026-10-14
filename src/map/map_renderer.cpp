#include "map_renderer.hpp"
#include "../qt/land_fill_path.hpp"
#include "coastline.hpp"
#include "land.hpp"
#include "path.hpp"
#include "projection.hpp"
#include <QLinearGradient>
#include <QFont>
#include <QPainter>
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <exception>
#include <iterator>
#include <stdexcept>
namespace hamclock {
MapRenderer::MapRenderer() {
    std::vector<LandFillPoint> points;
    std::vector<LandFillRing> rings;
    std::vector<LandFillRecord> records;
    for (const auto& p : land_points) points.push_back({p.longitude, p.latitude});
    for (const auto& r : land_rings) rings.push_back({r.start, r.count});
    for (const auto& r : land_records) records.push_back({r.first_ring, r.count});
    auto fill = make_land_fill_paths({points.data(), points.size(), rings.data(), rings.size(), records.data(), records.size()});
    if (fill.error != LandFillError::None) throw std::runtime_error("Bundled land geometry could not be prepared");
    land_ = std::move(fill.paths);
    for (const auto& part : coastline_parts)
        for (std::size_t i = 1; i < part.count; ++i) {
            const auto a = coastline_points[part.start + i - 1], b = coastline_points[part.start + i];
            const auto split = map_split_equirectangular({a.latitude, a.longitude}, {b.latitude, b.longitude});
            for (std::size_t j = 0; j < split.count; ++j) {
                coast_.moveTo(split.segments[j].first.x, split.segments[j].first.y);
                coast_.lineTo(split.segments[j].second.x, split.segments[j].second.y);
            }
        }
}
namespace {
// Gradient bar with title, end values and credit in the bottom-left corner.
void paint_legend(QPainter& p, QSizeF size, const MapStyle& st, bool dark) {
    const auto& stops = st.legend.stops;
    if (stops.size() < 2) return;
    const auto fm = p.fontMetrics();
    const double barw = std::clamp(size.width() * 0.25, 60.0, 180.0), barh = 8, pad = 4;
    const QString lo = QString::number(stops.front().value, 'g', 3), hi = QString::number(stops.back().value, 'g', 3);
    const QString head = st.legend.unit.isEmpty() ? st.legend.title : st.legend.title + " (" + st.legend.unit + ")";
    const double boxw = barw + 2 * pad, boxh = fm.height() * 2 + barh + 3 * pad;
    if (boxw > size.width() || boxh > size.height()) return;
    const QRectF box(pad, size.height() - boxh - pad, boxw, boxh);
    p.setPen(Qt::NoPen);
    p.setBrush(dark ? QColor(16, 24, 27, 200) : QColor(255, 255, 255, 200));
    p.drawRoundedRect(box, 3, 3);
    p.setPen(dark ? QColor("#e6eef0") : QColor("#1b2a30"));
    p.drawText(QPointF(box.left() + pad, box.top() + pad + fm.ascent()), head.isEmpty() ? st.name : head);
    const QRectF bar(box.left() + pad, box.top() + 2 * pad + fm.height(), barw, barh);
    QLinearGradient g(bar.left(), 0, bar.right(), 0);
    const double span = stops.back().value - stops.front().value;
    for (const auto& s : stops) g.setColorAt(span > 0 ? (s.value - stops.front().value) / span : 0.0, s.color);
    p.fillRect(bar, g);
    const double ty = bar.bottom() + fm.ascent() + 1;
    p.drawText(QPointF(bar.left(), ty), lo);
    p.drawText(QPointF(bar.right() - fm.horizontalAdvance(hi), ty), hi);
}
}
MapViewport MapRenderer::viewportFor(QSizeF size, const MapScene& scene) {
    std::optional<double> lo, hi;
    for (const auto& l : {scene.de, scene.dx})
        if (l) {
            lo = lo ? std::min(*lo, l->latitude) : l->latitude;
            hi = hi ? std::max(*hi, l->latitude) : l->latitude;
        }
    return MapViewport::fill(size.width(), size.height(), scene.de ? scene.de->longitude : 0.0, lo, hi,
                            scene.zoom, scene.pan_lon, scene.pan_lat);
}
void MapRenderer::paint(QPainter& painter, QSizeF size, const MapScene& scene) const {
    const bool dark = scene.dark;
    const QColor sea(dark ? "#374a51" : "#dce8eb"), coast(dark ? "#8b9fa3" : "#647c7e"), route(dark ? "#b4d9e8" : "#315e77");
    const QColor land(dark ? "#29363b" : "#d6ddce");
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(QRectF(QPointF(), size), sea);
    const auto vp = viewportFor(size, scene);
    if (vp.scale <= 0) { painter.restore(); return; }
    painter.setClipRect(QRectF(QPointF(), size));
    int first = 0, last = -1;
    vp.copy_range(first, last);
    const double W = vp.world_width(), H = vp.world_height();
    QPolygonF night;
    if (scene.sun) {
        auto pt = [](double lon, double lat) { return QPointF((lon + 180.0) / 360.0, (90.0 - lat) / 180.0); };
        const bool night_south = scene.sun->latitude_deg >= 0;
        for (int lon = -180; lon <= 180; ++lon) night << pt(lon, terminator_latitude_deg(*scene.sun, lon));
        night << pt(180, night_south ? -90 : 90) << pt(-180, night_south ? -90 : 90);
    }
    for (int k = first; k <= last; ++k) {
        QTransform t;
        t.translate(vp.origin_x() + k * W, vp.origin_y());
        t.scale(W, H);
        painter.save();
        painter.setTransform(t, true); // combine: keep the caller's base (contents/device) scale
        painter.setPen(Qt::NoPen);
        painter.setBrush(land);
        for (const auto& path : land_) painter.drawPath(path);
        QPen pen(coast); pen.setWidthF(1); pen.setCosmetic(true);
        painter.setPen(pen); painter.setBrush(Qt::NoBrush);
        painter.drawPath(coast_);
        if (scene.style && !scene.style->image.isNull()) {
            painter.setOpacity(std::clamp(scene.style->opacity, 0.0, 1.0));
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.drawImage(QRectF(0, 0, 1, 1), scene.style->image);
            painter.setOpacity(1.0);
        }
        if (scene.sun && scene.night_shade) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, dark ? 90 : 55));
            painter.drawPolygon(night);
        }
        painter.restore();
        auto px = [&](double nx, double ny) { return QPointF(vp.origin_x() + k * W + nx * W, vp.origin_y() + ny * H); };
        auto at = [&](Location l) { const auto p = map_project_equirectangular(l); return px(p.x, p.y); };
        const double ui = std::max(1.0, scene.ui_scale);
        { QFont f = painter.font(); f.setPixelSize(int(std::lround(11 * ui))); painter.setFont(f); }
        std::vector<QRectF> used_rects; // DE/DX first so they always win
        if (scene.sun) {
            const auto sun = at({scene.sun->latitude_deg, scene.sun->longitude_deg});
            painter.setPen(QPen(QColor(dark ? "#f2c14e" : "#d68f00"), 2)); painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(sun, 5, 5);
            painter.drawLine(QPointF(sun.x() - 8, sun.y()), QPointF(sun.x() + 8, sun.y()));
            painter.drawLine(QPointF(sun.x(), sun.y() - 8), QPointF(sun.x(), sun.y() + 8));
        }
        if (scene.route_visible) {
            painter.setPen(QPen(route, 2));
            for (std::size_t i = 1; i < scene.route.size(); ++i) {
                const auto split = map_split_equirectangular(scene.route[i - 1], scene.route[i]);
                for (std::size_t j = 0; j < split.count; ++j)
                    painter.drawLine(px(split.segments[j].first.x, split.segments[j].first.y),
                                     px(split.segments[j].second.x, split.segments[j].second.y));
            }
        }
        if (scene.de && scene.dx) {
            painter.setPen(QPen(route, 2)); painter.setBrush(sea);
            const auto de = at(*scene.de), dx = at(*scene.dx);
            const double dr = 4 * ui;
            painter.drawEllipse(de, dr, dr);
            painter.drawRect(QRectF(dx.x() - dr, dx.y() - dr, 2 * dr, 2 * dr));
            used_rects.push_back(QRectF(de.x() - dr, de.y() - dr, 2 * dr, 2 * dr));
            used_rects.push_back(QRectF(dx.x() - dr, dx.y() - dr, 2 * dr, 2 * dr));
            painter.setPen(route);
            const auto m = painter.fontMetrics();
            const QRectF view(QPointF(), size);
            for (const auto& item : {std::pair<QPointF, QString>{de, "DE"}, {dx, "DX"}}) {
                const double lw = m.horizontalAdvance(item.second);
                if (view.height() < m.height() || view.width() < lw || !view.contains(item.first)) continue;
                const QPointF base(std::clamp(item.first.x() + 7 * ui, 0.0, view.width() - lw),
                                   std::clamp(item.first.y() - 7 * ui, double(m.ascent()), view.height()));
                painter.drawText(base, item.second);
                used_rects.push_back(QRectF(base.x(), base.y() - m.ascent(), lw, m.height()));
            }
        }
        for (const auto& layer : scene.overlays) {
            if (!layer.visible) continue;
            const QRectF view(QPointF(), size);
            std::vector<QPointF> anchors;
            std::vector<const OverlayMarker*> labelled;
            for (const auto& m : layer.markers) {
                if (!valid_location(m.where)) continue;
                const auto p = at(m.where);
                if (m.path_from_de && scene.de) {
                    try {
                        const auto pts = map_short_path(*scene.de, m.where, 32);
                        QPen pp(m.color, 1); pp.setStyle(Qt::DashLine);
                        painter.setPen(pp);
                        for (std::size_t i = 1; i < pts.size(); ++i) {
                            const auto sp = map_split_equirectangular(pts[i - 1], pts[i]);
                            for (std::size_t j = 0; j < sp.count; ++j)
                                painter.drawLine(px(sp.segments[j].first.x, sp.segments[j].first.y),
                                                 px(sp.segments[j].second.x, sp.segments[j].second.y));
                        }
                    } catch (const std::exception&) {} // antipodal: no path invented
                }
                if (!view.adjusted(-m.size_px * ui, -m.size_px * ui, m.size_px * ui, m.size_px * ui).contains(p)) continue;
                painter.setPen(QPen(dark ? QColor("#10181b") : Qt::white, 1)); painter.setBrush(m.color);
                const double r = m.size_px * ui;
                switch (m.shape) {
                case OverlayShape::Dot: painter.drawEllipse(p, r, r); break;
                case OverlayShape::Square: painter.drawRect(QRectF(p.x() - r, p.y() - r, 2 * r, 2 * r)); break;
                case OverlayShape::Diamond: painter.drawPolygon(QPolygonF({{p.x(), p.y() - r}, {p.x() + r, p.y()}, {p.x(), p.y() + r}, {p.x() - r, p.y()}})); break;
                case OverlayShape::Triangle: painter.drawPolygon(QPolygonF({{p.x(), p.y() - r}, {p.x() + r, p.y() + r}, {p.x() - r, p.y() + r}})); break;
                }
                if (!overlay_label_text(m, layer.labels).isEmpty()) { anchors.push_back(p); labelled.push_back(&m); }
            }
            const auto keep = select_labels(anchors, layer.min_label_distance_px * ui);
            const auto fm = painter.fontMetrics();
            for (std::size_t i = 0; i < anchors.size(); ++i) {
                if (!keep[i]) continue;
                const auto text = overlay_label_text(*labelled[i], layer.labels);
                const auto rect = place_label(view, anchors[i], labelled[i]->size_px * ui,
                                              QSizeF(fm.horizontalAdvance(text), fm.height()), used_rects);
                if (!rect) continue;
                used_rects.push_back(*rect);
                painter.setPen(dark ? QColor("#e6eef0") : QColor("#1b2a30"));
                painter.drawText(QPointF(rect->left(), rect->top() + fm.ascent()), text);
            }
        }
    }
    if (scene.style) paint_legend(painter, size, *scene.style, dark);
    painter.restore();
}
}
