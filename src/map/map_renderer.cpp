#include "map_renderer.hpp"
#include "../qt/land_fill_path.hpp"
#include "coastline.hpp"
#include "land.hpp"
#include "projection.hpp"
#include <QPainter>
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
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
MapViewport MapRenderer::viewportFor(QSizeF size, const MapScene& scene) {
    std::optional<double> lo, hi;
    for (const auto& l : {scene.de, scene.dx})
        if (l) {
            lo = lo ? std::min(*lo, l->latitude) : l->latitude;
            hi = hi ? std::max(*hi, l->latitude) : l->latitude;
        }
    return MapViewport::fill(size.width(), size.height(), scene.de ? scene.de->longitude : 0.0, lo, hi);
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
        painter.setTransform(t, false);
        painter.setPen(Qt::NoPen);
        painter.setBrush(land);
        for (const auto& path : land_) painter.drawPath(path);
        QPen pen(coast); pen.setWidthF(1); pen.setCosmetic(true);
        painter.setPen(pen); painter.setBrush(Qt::NoBrush);
        painter.drawPath(coast_);
        if (scene.sun) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, dark ? 90 : 55));
            painter.drawPolygon(night);
        }
        painter.restore();
        auto px = [&](double nx, double ny) { return QPointF(vp.origin_x() + k * W + nx * W, vp.origin_y() + ny * H); };
        auto at = [&](Location l) { const auto p = map_project_equirectangular(l); return px(p.x, p.y); };
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
            painter.drawEllipse(de, 4, 4);
            painter.drawRect(QRectF(dx.x() - 4, dx.y() - 4, 8, 8));
            painter.setPen(route);
            const auto m = painter.fontMetrics();
            const QRectF view(QPointF(), size);
            for (const auto& item : {std::pair<QPointF, QString>{de, "DE"}, {dx, "DX"}}) {
                const double lw = m.horizontalAdvance(item.second);
                if (view.height() < m.height() || view.width() < lw || !view.contains(item.first)) continue;
                painter.drawText(QPointF(std::clamp(item.first.x() + 7, 0.0, view.width() - lw),
                                         std::clamp(item.first.y() - 7, double(m.ascent()), view.height())), item.second);
            }
        }
    }
    painter.restore();
}
}
