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
    v = MapViewport::fill(0, 0, 0);
    check(v.scale == 0, "empty size is harmless");

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
