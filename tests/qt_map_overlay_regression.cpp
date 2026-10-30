#include "../src/map/map_renderer.hpp"
#include "../src/map/overlay.hpp"
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <iostream>
using namespace hamclock;
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    int failures = 0;
    auto check = [&](bool ok, const char* name) { if (!ok) { ++failures; std::cerr << "Map overlay regression failed: " << name << '\n'; } };

    OverlayMarker m; m.call = "K1ABC"; m.prefix = "K";
    check(overlay_label_text(m, OverlayLabelMode::Call) == "K1ABC", "call label");
    check(overlay_label_text(m, OverlayLabelMode::Prefix) == "K", "prefix label");
    m.prefix.clear();
    check(overlay_label_text(m, OverlayLabelMode::Prefix) == "K1ABC", "prefix falls back to call");
    check(overlay_label_text(m, OverlayLabelMode::None).isEmpty() && overlay_label_text(m, OverlayLabelMode::Dot).isEmpty(), "None and Dot have no text");

    const auto keep = select_labels({{10, 10}, {30, 10}, {60, 10}, {61, 10}}, 40);
    check(keep == std::vector<bool>({true, false, true, false}), "minimum label distance, greedy in order");
    check(select_labels({{0, 0}, {1, 0}}, 0) == std::vector<bool>({true, true}), "zero distance keeps all");

    {
        const QRectF view(0, 0, 200, 100);
        const QSizeF t(30, 12);
        const auto right = place_label(view, {50, 50}, 4, t, {});
        check(right && right->left() == 57, "label goes right of the marker");
        const auto flip = place_label(view, {190, 50}, 4, t, {});
        check(flip && flip->right() <= 200 && flip->right() < 190, "label flips left at the right edge");
        const auto low = place_label(view, {50, 99}, 4, t, {});
        check(low && low->bottom() <= 100, "label clamped inside the bottom edge");
        check(!place_label(view, {50, 50}, 4, t, {QRectF(40, 40, 100, 20)}), "label skipped when both sides are taken");
        check(place_label(view, {50, 50}, 4, t, {QRectF(55, 40, 40, 20)}).has_value(), "falls back to the left side when the right is taken");
    }
    check(overlay_label_priority("satellites") < overlay_label_priority("dx-cluster")
          && overlay_label_priority("dx-cluster") < overlay_label_priority("quakes"), "label priority: satellites, spots, quakes");
    {
        const auto obs = line_obstacles({{0, 0}, {40, 0}, {-1e6, -1e6}, {80, 0}}, 100, 2);
        bool covers = false, jump = false;
        for (const auto& r : obs) { covers = covers || r.contains(QPointF(20, 0)); jump = jump || r.left() < -1000; }
        check(covers && !jump, "line obstacles follow the segment and skip off-map jumps");
        check(!place_label(QRectF(0, 0, 200, 100), {10, 0}, 4, QSizeF(30, 12), obs).has_value(), "label on a path line is dropped");
    }
    int calls = 0;
    auto& reg = MapOverlays::instance();
    const int token = reg.on_change([&] { ++calls; });
    OverlayLayer layer; layer.id = "spots"; layer.markers = {m};
    reg.set_layer(layer);
    reg.set_layer(layer);
    check(reg.layers().size() == 1 && calls == 2, "set_layer replaces by id and notifies");
    reg.remove_layer("spots");
    reg.remove_layer("spots");
    check(reg.layers().empty() && calls == 3, "remove_layer notifies once");
    reg.remove_listener(token);
    reg.set_layer(layer);
    check(calls == 3, "removed listener is not called");
    reg.remove_layer("spots");

    // Placement: with a 2:1 view the marker at 0,0 lands at the image centre.
    MapScene scene; scene.de = Location{40, -75};
    OverlayMarker at_origin; at_origin.where = {0, 0}; at_origin.color = QColor(255, 0, 0); at_origin.size_px = 5;
    OverlayMarker bad; bad.where = {200, 0}; // invalid: skipped, must not throw
    OverlayMarker anti; anti.where = {-40, 105}; anti.path_from_de = true; // antipode of DE: no path, no throw
    OverlayLayer l; l.id = "t"; l.labels = OverlayLabelMode::Dot; l.markers = {anti, bad, at_origin};
    scene.overlays = {l};
    scene.de = Location{0, 0}; // keep the crop symmetric
    QImage img(720, 360, QImage::Format_ARGB32);
    QPainter p(&img);
    MapRenderer().paint(p, QSizeF(720, 360), scene);
    p.end();
    check(img.pixelColor(360, 180) == QColor(255, 0, 0), "marker drawn at projected position");
    scene.overlays[0].visible = false;
    QImage img2(720, 360, QImage::Format_ARGB32);
    QPainter p2(&img2);
    MapRenderer().paint(p2, QSizeF(720, 360), scene);
    p2.end();
    check(img2.pixelColor(360, 180) != QColor(255, 0, 0), "hidden layer not drawn");
    {
        check(overlay_path_text(std::nullopt, {0, 0}).isEmpty(), "no DE, no path text");
        check(overlay_path_text(Location{0, 0}, {0, 90}) == "10008 km, 90°", "quarter circle east of DE");
        check(overlay_path_text(Location{0, 0}, {0, 0}) == "0 km", "same place has distance only");
        check(overlay_path_text(Location{0, 0}, {0, 180}) == "20015 km", "antipode has distance only");
    }
    {
        // Marker hit test: nearest visible marker within the radius, across world copies.
        const auto vp = MapViewport::fill(720, 360, 0.0);
        OverlayMarker a; a.where = {0, 0}; a.call = "A"; a.detail = "first";
        OverlayMarker b; b.where = {0, 179}; b.call = "B";
        OverlayLayer ly; ly.id = "h"; ly.markers = {a, b};
        std::vector<OverlayLayer> layers{ly};
        auto hit = hit_test_marker(layers, vp, vp.x_of(0) + 5, vp.y_of(0) - 3, 14);
        check(hit.marker && hit.marker->call == "A" && hit.marker->detail == "first" && hit.layer->id == "h", "hit test finds the marker");
        check(!hit_test_marker(layers, vp, vp.x_of(0) + 60, vp.y_of(0), 14).marker, "hit test misses outside the radius");
        hit = hit_test_marker(layers, vp, vp.x_of(-180) + 2, vp.y_of(0), 14);
        check(hit.marker && hit.marker->call == "B", "hit test wraps across the date line");
        layers[0].visible = false;
        check(!hit_test_marker(layers, vp, vp.x_of(0), vp.y_of(0), 14).marker, "hit test ignores hidden layers");
    }
    {
        // Issue #141: a painter with a base scale (Retina / contents scale) must place the
        // land exactly where the overlays go. Painting at 1x under scale(2) must match a direct 2x render.
        MapScene s2; s2.de = Location{51.5, -0.1}; s2.dx = Location{-33.9, 151.2};
        OverlayLayer l2; l2.id = "t"; OverlayMarker mk; mk.where = {21.3, -157.9}; mk.size_px = 4; mk.color = QColor(255, 0, 0);
        l2.markers = {mk}; s2.overlays = {l2};
        for (const QSizeF sz : {QSizeF(1024, 600), QSizeF(1024, 630)}) {
            QImage a(int(sz.width()) * 2, int(sz.height()) * 2, QImage::Format_ARGB32), b = a;
            QPainter pa(&a); pa.scale(2, 2); MapRenderer().paint(pa, sz, s2); pa.end();
            MapScene big = s2; big.ui_scale = 2.0;
            QPainter pb(&b); MapRenderer().paint(pb, QSizeF(sz.width() * 2, sz.height() * 2), big); pb.end();
            long diff = 0;
            for (int y = 0; y < a.height(); y += 2)
                for (int x = 0; x < a.width(); x += 2)
                    if (qGray(a.pixel(x, y)) / 24 != qGray(b.pixel(x, y)) / 24) ++diff;
            check(diff * 50 < long(a.width()) * a.height() / 4, "base-scaled painter matches direct render (land and overlays share one transform)");
        }
    }
    {
        // Issue #423: two bodies ~40 px apart (Venus, Mercury) must never overprint labels.
        const QRectF view(0, 0, 400, 100);
        const QSizeF t(36, 12);
        const QPointF venus(100, 50), mercury(140, 50);
        std::vector<QRectF> used = {QRectF(venus.x() - 4, venus.y() - 4, 8, 8), QRectF(mercury.x() - 4, mercury.y() - 4, 8, 8)};
        const auto first = place_label(view, venus, 4, t, used);
        check(first.has_value() && !first->intersects(used[1]), "first planet label clears the neighbouring marker");
        if (first) used.push_back(*first);
        const auto second = place_label(view, mercury, 4, QSizeF(48, 12), used);
        check(!second || !second->intersects(*first), "second planet label never overprints the first");
    }
    {
        // No DX target: DE is still marked (it used to vanish with the DX).
        MapScene s; s.de = Location{0, 0}; s.night_shade = false;
        QImage img(720, 360, QImage::Format_ARGB32);
        QPainter p(&img); MapRenderer().paint(p, QSizeF(720, 360), s); p.end();
        int ring = 0;
        for (int y = 174; y <= 186; ++y)
            for (int x = 354; x <= 366; ++x) ring += img.pixelColor(x, y).lightness() < 120;
        check(ring > 6, "DE marker drawn without a DX");
    }
    return failures == 0 ? 0 : 1;
}
