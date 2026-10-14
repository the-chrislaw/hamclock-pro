#include "map_renderer.hpp"
#include "../qt/land_fill_path.hpp"
#include "borders.hpp"
#include "coastline.hpp"
#include "land.hpp"
#include "path.hpp"
#include "projection.hpp"
#include "sky_points.hpp"
#include <QLinearGradient>
#include <QFont>
#include <QPainter>
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <exception>
#include <functional>
#include <iterator>
#include <QImage>
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
    for (const auto& part : border_parts)
        for (std::size_t i = 1; i < part.count; ++i) {
            const auto a = border_points[part.start + i - 1], b = border_points[part.start + i];
            const auto split = map_split_equirectangular({a.latitude, a.longitude}, {b.latitude, b.longitude});
            for (std::size_t j = 0; j < split.count; ++j) {
                borders_.moveTo(split.segments[j].first.x, split.segments[j].first.y);
                borders_.lineTo(split.segments[j].second.x, split.segments[j].second.y);
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
// How the vector layers (sun, DE/DX, paths, overlays) reach the screen: the
// equirectangular and the whole-world projections differ only in these two.
struct GeoDraw {
    std::function<std::optional<QPointF>(Location)> at;
    std::function<void(QPainter&, Location, Location)> line; // current pen, seam-safe
};
void paint_geo_layers(QPainter& painter, QSizeF size, const MapScene& scene, const GeoDraw& geo, const QColor& sea, const QColor& route_color) {
    const bool dark = scene.dark;
    const QColor route = route_color;
    const double ui = std::max(1.0, scene.ui_scale);
    { QFont f = painter.font(); f.setPixelSize(int(std::lround(11 * ui))); painter.setFont(f); }
    std::vector<QRectF> used_rects = scene.reserved; // map buttons, then DE/DX, so they always win
    if (scene.sun) {
        if (const auto sun = geo.at({scene.sun->latitude_deg, scene.sun->longitude_deg})) {
            painter.setPen(QPen(QColor(dark ? "#f2c14e" : "#d68f00"), 2)); painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(*sun, 5, 5);
            painter.drawLine(QPointF(sun->x() - 8, sun->y()), QPointF(sun->x() + 8, sun->y()));
            painter.drawLine(QPointF(sun->x(), sun->y() - 8), QPointF(sun->x(), sun->y() + 8));
        }
    }
    if (scene.sky_points) {
        const QColor c(dark ? "#d8dde6" : "#59606e");
        painter.setBrush(Qt::NoBrush);
        if (scene.moon) { // open circle with a crescent-ish inner dot
            if (const auto p = geo.at(*scene.moon)) {
                painter.setPen(QPen(c, 2)); painter.drawEllipse(*p, 5 * ui, 5 * ui);
                painter.setBrush(c); painter.drawEllipse(*p, 2 * ui, 2 * ui);
            }
        }
        if (scene.de) { // antipode: X
            if (const auto p = geo.at(antipode(*scene.de))) {
                const double r = 4 * ui;
                painter.setPen(QPen(c, 2));
                painter.drawLine(QPointF(p->x() - r, p->y() - r), QPointF(p->x() + r, p->y() + r));
                painter.drawLine(QPointF(p->x() - r, p->y() + r), QPointF(p->x() + r, p->y() - r));
            }
        }
    }
    if (scene.route_visible) {
        painter.setPen(QPen(route, 2));
        for (std::size_t i = 1; i < scene.route.size(); ++i) geo.line(painter, scene.route[i - 1], scene.route[i]);
        std::vector<QPointF> px;
        for (const auto& l : scene.route) { if (const auto p = geo.at(l)) px.push_back(*p); else px.push_back({-1e6, -1e6}); }
        const auto line = line_obstacles(px, std::max(size.width(), size.height()) / 2, 2 * ui);
        used_rects.insert(used_rects.end(), line.begin(), line.end());
    }
    if (scene.grid == MapGrid::AzimuthRings && scene.de) {
        QPen gp(dark ? QColor(230, 238, 240, 110) : QColor(27, 42, 48, 100)); gp.setWidthF(1); gp.setCosmetic(true);
        painter.setPen(gp);
        for (double d = azimuth_ring_step_deg; d < 180.0; d += azimuth_ring_step_deg) {
            const auto ring = ring_points(scene.de->latitude, scene.de->longitude, d);
            for (std::size_t i = 1; i < ring.size(); ++i)
                geo.line(painter, {ring[i - 1].lat, ring[i - 1].lon}, {ring[i].lat, ring[i].lon});
        }
    }
    const auto de_px = scene.de ? geo.at(*scene.de) : std::nullopt;
    const auto dx_px = scene.dx ? geo.at(*scene.dx) : std::nullopt;
    if (de_px && dx_px) {
        painter.setPen(QPen(route, 2)); painter.setBrush(sea);
        const auto de = *de_px, dx = *dx_px;
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
            // Prefer the first corner clear of the path and controls.
            const double o = 7 * ui;
            QPointF base;
            bool first = true;
            for (const QPointF off : {QPointF(o, -o), QPointF(-o - lw, -o), QPointF(o, o + m.ascent()), QPointF(-o - lw, o + m.ascent())}) {
                const QPointF b(std::clamp(item.first.x() + off.x(), 0.0, view.width() - lw),
                                std::clamp(item.first.y() + off.y(), double(m.ascent()), view.height()));
                const QRectF r(b.x(), b.y() - m.ascent(), lw, m.height());
                bool clash = false;
                for (const auto& u : used_rects) clash = clash || u.intersects(r);
                if (first || !clash) base = b;
                first = false;
                if (!clash) break;
            }
            painter.drawText(base, item.second);
            used_rects.push_back(QRectF(base.x(), base.y() - m.ascent(), lw, m.height()));
        }
    }
    // Symbols of every layer first, so labels of any layer avoid all of them;
    // then labels by layer priority (satellites, spots, hazards).
    const QRectF view(QPointF(), size);
    struct Pending { const OverlayLayer* layer; std::vector<QPointF> anchors; std::vector<const OverlayMarker*> labelled; };
    std::vector<Pending> pending;
    for (const auto& layer : scene.overlays) {
        if (!layer.visible) continue;
        pending.push_back({&layer, {}, {}});
        auto& anchors = pending.back().anchors;
        auto& labelled = pending.back().labelled;
        for (const auto& m : layer.markers) {
            if (!valid_location(m.where)) continue;
            const auto at_m = geo.at(m.where);
            if (m.path_from_de && scene.de) {
                try {
                    const auto pts = map_short_path(*scene.de, m.where, 32);
                    QPen pp(m.color, 1); pp.setStyle(Qt::DashLine);
                    painter.setPen(pp);
                    for (std::size_t i = 1; i < pts.size(); ++i) geo.line(painter, pts[i - 1], pts[i]);
                } catch (const std::exception&) {} // antipodal: no path invented
            }
            if (!at_m) continue;
            const QPointF p = *at_m;
            if (!view.adjusted(-m.size_px * ui, -m.size_px * ui, m.size_px * ui, m.size_px * ui).contains(p)) continue;
            painter.setPen(QPen(dark ? QColor("#10181b") : Qt::white, 1)); painter.setBrush(m.color);
            const double r = m.size_px * ui;
            used_rects.push_back(QRectF(p.x() - r, p.y() - r, 2 * r, 2 * r));
            switch (m.shape) {
            case OverlayShape::Dot: painter.drawEllipse(p, r, r); break;
            case OverlayShape::Square: painter.drawRect(QRectF(p.x() - r, p.y() - r, 2 * r, 2 * r)); break;
            case OverlayShape::Diamond: painter.drawPolygon(QPolygonF({{p.x(), p.y() - r}, {p.x() + r, p.y()}, {p.x(), p.y() + r}, {p.x() - r, p.y()}})); break;
            case OverlayShape::Triangle: painter.drawPolygon(QPolygonF({{p.x(), p.y() - r}, {p.x() + r, p.y() + r}, {p.x() - r, p.y() + r}})); break;
            }
            if (!overlay_label_text(m, layer.labels).isEmpty()) { anchors.push_back(p); labelled.push_back(&m); }
        }
    }
    std::stable_sort(pending.begin(), pending.end(), [](const Pending& x, const Pending& y) {
        return overlay_label_priority(x.layer->id) < overlay_label_priority(y.layer->id);
    });
    for (const auto& pl : pending) {
        const auto& layer = *pl.layer;
        const auto& anchors = pl.anchors;
        const auto& labelled = pl.labelled;
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
}
namespace {
// Bilinear lookup in the equirectangular base image (wraps in longitude).
QRgb sample_base(const QImage& img, double lat, double lon) {
    const int w = img.width(), h = img.height();
    const double u = (lon + 180.0) / 360.0 * w - 0.5, v = (90.0 - lat) / 180.0 * h - 0.5;
    const int x0 = static_cast<int>(std::floor(u)), y0 = static_cast<int>(std::floor(v));
    const double fx = u - x0, fy = v - y0;
    auto at = [&](int x, int y) { return reinterpret_cast<const QRgb*>(img.constScanLine(std::clamp(y, 0, h - 1)))[((x % w) + w) % w]; };
    const QRgb c[4] = {at(x0, y0), at(x0 + 1, y0), at(x0, y0 + 1), at(x0 + 1, y0 + 1)};
    const double wt[4] = {(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
    auto ch = [&](int shift) {
        double sum = 0;
        for (int i = 0; i < 4; ++i) sum += wt[i] * ((c[i] >> shift) & 0xff);
        return static_cast<int>(sum + 0.5);
    };
    return qRgb(ch(16), ch(8), ch(0));
}
}
QImage MapRenderer::equirect_base(const MapScene& scene, const QColor& sea, const QColor& land, const QColor& coast) const {
    const bool styled = scene.style && !scene.style->image.isNull();
    const QString key = QStringLiteral("%1|%2|%3|%4|%5").arg(scene.dark).arg(scene.gray)
                            .arg(styled ? scene.style->image.cacheKey() : 0).arg(styled ? scene.style->opacity : 0.0).arg(scene.borders);
    if (base_key_ == key && !base_.isNull()) return base_;
    QImage img(2048, 1024, QImage::Format_RGB32);
    img.fill(sea);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(img.width(), img.height());
    p.setPen(Qt::NoPen);
    p.setBrush(land);
    for (const auto& path : land_) p.drawPath(path);
    QPen pen(coast); pen.setWidthF(1.5 / img.width()); // 1.5 px of the base image
    p.setPen(pen); p.setBrush(Qt::NoBrush);
    p.drawPath(coast_);
    if (scene.borders) {
        QColor bc = coast; bc.setAlpha(150);
        QPen bp(bc); bp.setWidthF(1.0 / img.width()); bp.setStyle(Qt::DashLine);
        p.setPen(bp);
        p.drawPath(borders_);
    }
    if (styled) {
        p.setOpacity(std::clamp(scene.style->opacity, 0.0, 1.0));
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.drawImage(QRectF(0, 0, 1, 1), scene.style->image);
    }
    p.end();
    base_ = img;
    base_key_ = key;
    return base_;
}
MapFrame MapRenderer::frameFor(QSizeF size, const MapScene& scene) {
    Location centre{0, scene.center_lon ? *scene.center_lon : 0.0};
    if (scene.projection == MapProjection::Azimuthal && scene.de) centre = *scene.de;
    return MapFrame::fit(scene.projection, centre, size.width(), size.height());
}
// Whole-world projections: the base map is resampled per pixel from the
// equirectangular image through the frame's inverse, so land, night shade,
// markers and taps all use the same transform.
void MapRenderer::paint_projected(QPainter& painter, QSizeF size, const MapScene& scene, const MapFrame& frame,
                                  const QColor& sea, const QColor& land, const QColor& coast, const QColor& route) const {
    const bool dark = scene.dark;
    painter.fillRect(QRectF(QPointF(), size), dark ? QColor("#10181b") : QColor("#eef3f4"));
    if (frame.w <= 0) return;
    const QImage base = equirect_base(scene, sea, land, coast);
    const double dev = std::clamp(painter.deviceTransform().m11(), 1.0, 3.0);
    const double sc = std::min(dev, std::sqrt(1.5e6 / (frame.w * frame.h))); // cap the per-pixel work
    const int iw = std::max(1, int(std::lround(frame.w * sc))), ih = std::max(1, int(std::lround(frame.h * sc)));
    const bool night = scene.sun && scene.night_shade;
    const QString key = QStringLiteral("%1|%2|%3|%4|%5|%6|%7|%8|%9").arg(base_key_).arg(int(frame.kind)).arg(frame.centre.latitude)
        .arg(frame.centre.longitude).arg(iw).arg(ih).arg(night ? scene.sun->latitude_deg : 0.0)
        .arg(night ? scene.sun->longitude_deg : 0.0).arg(night);
    if (key != proj_key_ || proj_.isNull()) {
        QImage out(iw, ih, QImage::Format_ARGB32_Premultiplied);
        out.fill(Qt::transparent);
        constexpr double rad = 3.14159265358979323846 / 180.0;
        const double sd = night ? std::sin(scene.sun->latitude_deg * rad) : 0, cd = night ? std::cos(scene.sun->latitude_deg * rad) : 0;
        const double shade = 1.0 - (dark ? 90.0 : 55.0) / 255.0;
        for (int j = 0; j < ih; ++j) {
            auto* line = reinterpret_cast<QRgb*>(out.scanLine(j));
            const double y = frame.y + (j + 0.5) / ih * frame.h;
            for (int i = 0; i < iw; ++i) {
                const auto loc = frame.to_location(frame.x + (i + 0.5) / iw * frame.w, y);
                if (!loc) continue;
                QRgb c = sample_base(base, loc->latitude, loc->longitude);
                if (night) {
                    const double cz = std::sin(loc->latitude * rad) * sd +
                                      std::cos(loc->latitude * rad) * cd * std::cos((loc->longitude - scene.sun->longitude_deg) * rad);
                    if (cz < 0) c = qRgb(int(qRed(c) * shade), int(qGreen(c) * shade), int(qBlue(c) * shade));
                }
                line[i] = c | 0xff000000u;
            }
        }
        proj_ = out;
        proj_key_ = key;
    }
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(QRectF(frame.x, frame.y, frame.w, frame.h), proj_);
    QPen outline(coast); outline.setWidthF(1); outline.setCosmetic(true);
    painter.setPen(outline); painter.setBrush(Qt::NoBrush);
    if (frame.kind == MapProjection::Azimuthal) {
        painter.drawEllipse(QRectF(frame.x, frame.y, frame.w, frame.h));
    } else {
        QPolygonF edge;
        for (int lat = 90; lat >= -90; lat -= 3) { const auto p = map_project_robinson({double(lat), -180}); edge << QPointF(frame.x + p.x * frame.w, frame.y + p.y * frame.h); }
        for (int lat = -90; lat <= 90; lat += 3) { const auto p = map_project_robinson({double(lat), 180}); edge << QPointF(frame.x + p.x * frame.w, frame.y + p.y * frame.h); }
        painter.drawPolygon(edge);
    }
    const double jump = 0.25 * std::max(frame.w, frame.h); // longer hops cross the projection's seam
    const GeoDraw geo{
        [&](Location l) -> std::optional<QPointF> {
            const auto p = frame.to_pixel(l);
            return p ? std::optional<QPointF>(QPointF(p->x, p->y)) : std::nullopt;
        },
        [&](QPainter& pa, Location a, Location b) {
            const auto pa_ = frame.to_pixel(a), pb = frame.to_pixel(b);
            if (pa_ && pb && std::hypot(pa_->x - pb->x, pa_->y - pb->y) < jump)
                pa.drawLine(QPointF(pa_->x, pa_->y), QPointF(pb->x, pb->y));
        }};
    if (scene.grid != MapGrid::None && scene.grid != MapGrid::AzimuthRings) {
        QPen gp(dark ? QColor(230, 238, 240, 90) : QColor(27, 42, 48, 80)); gp.setWidthF(1); gp.setCosmetic(true);
        painter.setPen(gp);
        for (const auto& l : grid_lines(scene.grid)) {
            const int n = std::max(1, int(std::ceil(std::max(std::abs(l.lat2 - l.lat1), std::abs(l.lon2 - l.lon1)) / 2.0)));
            Location prev{l.lat1, l.lon1};
            for (int i = 1; i <= n; ++i) {
                const Location next{l.lat1 + (l.lat2 - l.lat1) * i / n, l.lon1 + (l.lon2 - l.lon1) * i / n};
                geo.line(painter, prev, next);
                prev = next;
            }
        }
    }
    painter.save();
    painter.setClipRect(QRectF(QPointF(), size));
    paint_geo_layers(painter, size, scene, geo, sea, route);
    painter.restore();
}
MapViewport MapRenderer::viewportFor(QSizeF size, const MapScene& scene) {
    std::optional<double> lo, hi;
    for (const auto& l : {scene.de, scene.dx})
        if (l) {
            lo = lo ? std::min(*lo, l->latitude) : l->latitude;
            hi = hi ? std::max(*hi, l->latitude) : l->latitude;
        }
    return MapViewport::fill(size.width(), size.height(), scene.de ? scene.de->longitude : 0.0, lo, hi,
                            scene.zoom, scene.pan_lon ? scene.pan_lon : scene.center_lon, scene.pan_lat);
}
void MapRenderer::paint(QPainter& painter, QSizeF size, const MapScene& scene) const {
    const bool dark = scene.dark;
    QColor sea(dark ? "#374a51" : "#dce8eb"), coast(dark ? "#8b9fa3" : "#647c7e");
    QColor land(dark ? "#29363b" : "#d6ddce");
    if (scene.gray) // base map only; overlays keep their colours
        for (QColor* c : {&sea, &coast, &land}) { const int g = qGray(c->rgb()); *c = QColor(g, g, g); }
    const QColor route(dark ? "#b4d9e8" : "#315e77");
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    if (scene.projection != MapProjection::Equirectangular) {
        paint_projected(painter, size, scene, frameFor(size, scene), sea, land, coast, route);
        if (scene.style) paint_legend(painter, size, *scene.style, dark);
        painter.restore();
        return;
    }
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
        if (scene.borders) {
            QColor bc = coast; bc.setAlpha(150);
            QPen bp(bc); bp.setWidthF(1); bp.setCosmetic(true); bp.setStyle(Qt::DashLine);
            painter.setPen(bp);
            painter.drawPath(borders_);
        }
        if (scene.style && !scene.style->image.isNull()) {
            painter.setOpacity(std::clamp(scene.style->opacity, 0.0, 1.0));
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.drawImage(QRectF(0, 0, 1, 1), scene.style->image);
            painter.setOpacity(1.0);
        }
        if (scene.grid != MapGrid::None) {
            QPen gp(dark ? QColor(230, 238, 240, 90) : QColor(27, 42, 48, 80)); gp.setWidthF(1); gp.setCosmetic(true);
            painter.setPen(gp); painter.setBrush(Qt::NoBrush);
            for (const auto& l : grid_lines(scene.grid))
                painter.drawLine(QPointF((l.lon1 + 180.0) / 360.0, (90.0 - l.lat1) / 180.0),
                                 QPointF((l.lon2 + 180.0) / 360.0, (90.0 - l.lat2) / 180.0));
        }
        if (scene.sun && scene.night_shade) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, dark ? 90 : 55));
            painter.drawPolygon(night);
        }
        painter.restore();
        const auto px = [&](double nx, double ny) { return QPointF(vp.origin_x() + k * W + nx * W, vp.origin_y() + ny * H); };
        const GeoDraw geo{
            [&](Location l) -> std::optional<QPointF> {
                if (!valid_location(l)) return std::nullopt;
                const auto p = map_project_equirectangular(l);
                return px(p.x, p.y);
            },
            [&](QPainter& pa, Location a, Location b) {
                const auto split = map_split_equirectangular(a, b);
                for (std::size_t j = 0; j < split.count; ++j)
                    pa.drawLine(px(split.segments[j].first.x, split.segments[j].first.y),
                                px(split.segments[j].second.x, split.segments[j].second.y));
            }};
        paint_geo_layers(painter, size, scene, geo, sea, route);
    }
    if (scene.style) paint_legend(painter, size, *scene.style, dark);
    painter.restore();
}
}
