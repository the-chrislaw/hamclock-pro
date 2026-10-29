#include "../src/map/map_renderer.hpp"
#include "../src/map/viewport.hpp"
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <cmath>
#include <iostream>
using namespace hamclock;
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    int failures = 0;
    auto check = [&](bool ok, const char* name) { if (!ok) { ++failures; std::cerr << "Map viewport regression failed: " << name << '\n'; } };
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-9; };

    // Exact 2:1 shows the whole world.
    auto v = MapViewport::fill(720, 360, 0);
    check(near(v.scale, 2) && near(v.origin_x(), 0) && near(v.origin_y(), 0), "2:1 shows the whole world");
    check(near(v.x_of(0), 360) && near(v.y_of(0), 180) && near(v.x_of(-180), 0) && near(v.y_of(90), 0), "known coordinates");
    // 1024x600 (map card is wider than 2:1 once chrome is removed): fills, crops polar rows, keeps +/-70.
    for (const QSizeF size : {QSizeF(1024, 600), QSizeF(800, 480), QSizeF(1440, 555), QSizeF(1920, 400), QSizeF(300, 500)}) {
        v = MapViewport::fill(size.width(), size.height(), 0);
        int a = 0, b = -1;
        v.copy_range(a, b);
        check(v.origin_x() + a * v.world_width() <= 1e-6 && v.origin_x() + (b + 1) * v.world_width() >= size.width() - 1e-6, "world copies cover the width");
        check(v.visible_half_latitude() >= 70.0 - 1e-9 || v.world_height() >= size.height() - 1e-6, "keeps +/-70 degrees");
        check(v.origin_y() <= 1e-6 && v.origin_y() + v.world_height() >= size.height() - 1e-6, "world covers the height");
    }
    v = MapViewport::fill(1440, 555, 0);
    check(near(v.scale, 555.0 / 140.0), "very wide pane clamps at +/-70 and wraps longitude");
    int first = 0, last = 0;
    v.copy_range(first, last);
    check(first == -1 && last == 1, "wrapped copy range covers the item");
    v = MapViewport::fill(1000, 500, 0);
    v.copy_range(first, last);
    check(first == 0 && last == 0, "single copy at 2:1");
    // Tall pane crops longitude around the preferred centre.
    v = MapViewport::fill(300, 500, 120);
    check(near(v.center_lon, 120) && near(v.x_of(120), 150), "tall pane centres the preferred longitude");
    check(near(v.x_of(120 + 360), v.x_of(120)), "longitude wraps");
    // Focus latitude shifts the crop but stays inside the world.
    v = MapViewport::fill(1024, 600, 0, 75.0, 80.0);
    check(v.y_of(77.5) > 0 && v.y_of(77.5) < 600 && v.origin_y() <= 1e-6, "polar DE/DX stay visible");
    // Zoom, pan and inverse (tap) maths.
    v = MapViewport::fill(1024, 600, 0);
    const double base_scale = v.scale;
    auto z = MapViewport::fill(1024, 600, 0, std::nullopt, std::nullopt, 2.0, 10.0, 20.0);
    check(near(z.scale, 2 * base_scale), "zoom 2 doubles the scale");
    check(near(z.x_of(10.0), 512) && near(z.y_of(20.0), 300), "pan centre sits mid-item");
    const auto at = z.location_at(512, 300);
    check(near(at.lat, 20.0) && near(at.lon, 10.0), "tap at centre returns the centre");
    const auto back = z.location_at(z.x_of(-100.0), z.y_of(45.0));
    check(near(back.lat, 45.0) && near(back.lon, -100.0), "location_at inverts x_of/y_of");
    z = MapViewport::fill(1024, 600, 0, std::nullopt, std::nullopt, 99.0, 0.0, 89.0);
    check(near(z.scale, 4 * base_scale), "zoom clamps to 4");
    check(z.origin_y() <= 1e-6 && z.origin_y() + z.world_height() >= 600 - 1e-6, "pan keeps the world covering the item");
    z = MapViewport::fill(1024, 600, 0, std::nullopt, std::nullopt, 0.1);
    check(near(z.scale, base_scale), "zoom below 1 clamps to 1");
    z = MapViewport::fill(1024, 600, 55.0, std::nullopt, std::nullopt, 3.0);
    check(near(z.center_lon, 55.0), "zoomed view centres on the preferred longitude");
    // Zoom about a gesture point keeps the location under the finger (card sizes 800x310..1920x1080).
    for (const auto& sz : {std::pair{800.0, 310.0}, {1024.0, 600.0}, {1920.0, 1080.0}}) {
        const auto b = MapViewport::fill(sz.first, sz.second, 0);
        const double px = sz.first * 0.7, py = sz.second * 0.6;
        const auto loc = b.location_at(px, py);
        const auto z1 = MapViewport::fill(sz.first, sz.second, 0, std::nullopt, std::nullopt, 2.5);
        const auto c = z1.center_for_anchor(loc, px, py);
        const auto z2 = MapViewport::fill(sz.first, sz.second, 0, std::nullopt, std::nullopt, 2.5, c.lon, c.lat);
        const auto got = z2.location_at(px, py);
        check(near(got.lat, loc.lat) && near(got.lon, loc.lon), "zoom about a point keeps the anchored location");
    }
    v = MapViewport::fill(0, 0, 0);
    check(v.scale == 0, "empty size is harmless");

    // iPad portrait (map card ~1:0.9): longitude is cropped at zoom 1, so it can pan sideways.
    v = MapViewport::fill(820, 760, -72.7);
    check(v.longitude_cropped(), "tall pane crops longitude at zoom 1");
    check(!MapViewport::fill(1440, 555, 0).longitude_cropped(), "very wide pane wraps instead of cropping");
    // W1AW to Tokyo: DE-centred crop hides Tokyo; the auto centre shows both.
    check(std::fabs(v.x_of(139.69) - 410) > 410, "DE-centred tall crop hides a far DX");
    const double c = MapViewport::auto_center_lon(820, 760, -72.7, 139.69);
    v = MapViewport::fill(820, 760, c);
    check(v.x_of(-72.7) > 0 && v.x_of(-72.7) < 820 && v.x_of(139.69) > 0 && v.x_of(139.69) < 820, "auto centre shows DE and DX");
    check(near(MapViewport::auto_center_lon(820, 760, -72.7, std::nullopt), -72.7), "no DX centres on DE");
    // DE and Jakarta (~180 degrees apart) cannot both fit with a margin: stay on DE.
    check(near(MapViewport::auto_center_lon(820, 760, -72.7, 106.8), -72.7), "DX too far to fit keeps DE centred");
    check(near(MapViewport::auto_center_lon(1440, 555, -72.7, 139.69), -72.7), "uncropped pane keeps DE centre");

    // Zoom about a point: the location under the gesture stays under it, at several sizes and zooms.
    for (const QSize size : {QSize(1024, 600), QSize(800, 480), QSize(1920, 1080)}) {
        for (const double z : {1.5, 2.0, 4.0}) {
            const auto base = MapViewport::fill(size.width(), size.height(), 0.0);
            const double px = size.width() * 0.3, py = size.height() * 0.6;
            const auto at = base.location_at(px, py);
            auto zoomed = MapViewport::fill(size.width(), size.height(), 0.0, std::nullopt, std::nullopt, z);
            const auto c = zoomed.center_for_anchor(at, px, py);
            zoomed = MapViewport::fill(size.width(), size.height(), 0.0, std::nullopt, std::nullopt, z, c.lon, c.lat);
            const auto back = zoomed.location_at(px, py);
            // Pan is clamped so the world still covers the item; unclamped anchors must hold exactly.
            const bool clamped = std::fabs(zoomed.center_lat - c.lat) > 1e-9;
            check(clamped || (std::fabs(back.lat - at.lat) < 1e-6 && std::fabs(back.lon - at.lon) < 1e-6), "zoom keeps the anchor under the gesture");
            check(std::fabs(zoomed.scale / base.scale - z) < 1e-9, "zoom multiplies the covering scale");
        }
    }

    // Rendering: every column of the pane carries map content (not flat sea).
    MapScene scene;
    scene.de = Location{37.77, -122.42};
    scene.dx = Location{35.68, 139.69};
    scene.route = {*scene.de, *scene.dx};
    scene.sun = SubsolarPoint{10.0, 20.0};
    MapRenderer renderer;
    for (const QSize size : {QSize(1024, 600), QSize(800, 480), QSize(1440, 555), QSize(300, 500), QSize(16, 16)}) {
        QImage image(size, QImage::Format_ARGB32);
        image.fill(Qt::magenta);
        QPainter painter(&image);
        renderer.paint(painter, QSizeF(size), scene);
        painter.end();
        bool magenta = false;
        for (int y = 0; y < size.height() && !magenta; ++y)
            for (int x = 0; x < size.width(); ++x)
                if (image.pixel(x, y) == QColor(Qt::magenta).rgba()) { magenta = true; break; }
        check(!magenta, "map paints every pixel of the item");
    }
    return failures ? 1 : 0;
}
