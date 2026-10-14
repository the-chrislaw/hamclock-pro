#include "../src/map/view_options.hpp"
#include "../src/map/map_renderer.hpp"
#include <QGuiApplication>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <iostream>
using namespace hamclock;
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    int failures = 0;
    auto check = [&](bool ok, const char* name) { if (!ok) { ++failures; std::cerr << "Map view options regression failed: " << name << '\n'; } };

    OverlayLayer a; a.id = "spots"; a.labels = OverlayLabelMode::Call;
    OverlayLayer b; b.id = "quakes"; b.labels = OverlayLabelMode::Prefix;
    const std::vector<OverlayLayer> layers{a, b};

    MapViewOptions o;
    check(o.layer_visible("spots") && o.night_shade && o.path && o.label_mode == -1, "defaults");
    auto same = o.apply(layers);
    check(same[0].visible && same[1].visible && same[0].labels == OverlayLabelMode::Call && same[1].labels == OverlayLabelMode::Prefix, "defaults leave layers alone");

    o.set_layer_visible("quakes", false);
    o.label_mode = static_cast<int>(OverlayLabelMode::Dot);
    auto applied = o.apply(layers);
    check(applied[0].visible && !applied[1].visible, "hidden layer");
    check(applied[0].labels == OverlayLabelMode::Dot && applied[1].labels == OverlayLabelMode::Dot, "label override");
    check(layers[1].visible, "input untouched");
    o.set_layer_visible("quakes", true);
    check(o.layer_visible("quakes"), "re-enable");

    // A layer hidden before it is published stays hidden once it appears.
    o.set_layer_visible("satellites", false);
    OverlayLayer s; s.id = "satellites";
    check(!o.apply({s})[0].visible, "hidden by id before publication");

    QTemporaryDir dir;
    const QString ini = dir.filePath("options.ini");
    o.night_shade = false; o.path = false;
    check(o.save(ini), "save");
    MapViewOptions r; r.load(ini);
    check(r.hidden_layers == o.hidden_layers && r.label_mode == 3 && !r.night_shade && !r.path, "round trip");
    check(r.style.isEmpty() && r.rotation_s == 0, "style defaults");
    o.style = "aurora"; o.rotation_s = style_rotation_intervals().back();
    check(o.save(ini), "save style");
    MapViewOptions rs; rs.load(ini);
    check(rs.style == "aurora" && rs.rotation_s == o.rotation_s, "style round trip");
    { QFile g(ini); g.open(QIODevice::WriteOnly | QIODevice::Append); g.write("\n[map]\nrotation=7\n"); }
    MapViewOptions rb; rb.load(ini);
    check(rb.rotation_s == 0, "unknown rotation interval rejected");
    MapViewOptions missing; missing.load(dir.filePath("none.ini"));
    check(missing.layer_visible("spots") && missing.night_shade && missing.label_mode == -1, "missing file gives defaults");

    QFile f(ini); f.open(QIODevice::WriteOnly | QIODevice::Append); f.write("\n[map]\nlabels=99\n"); f.close();
    MapViewOptions bad; bad.load(ini);
    check(bad.label_mode == -1 || (bad.label_mode >= 0 && bad.label_mode <= 3), "out-of-range label mode rejected");

    auto render = [&](const MapScene& m) { QImage img(400, 240, QImage::Format_ARGB32); img.fill(Qt::white); QPainter p(&img); MapRenderer().paint(p, QSizeF(400, 240), m); return img; };
    // Grid: persisted, out-of-range rejected, lines at known coordinates, drawn.
    o.grid = 3; check(o.save(ini), "save grid");
    MapViewOptions rg; rg.load(ini); check(rg.grid == 3, "grid round trip");
    { QFile g(ini); g.open(QIODevice::WriteOnly | QIODevice::Append); g.write("\n[map]\ngrid=9\n"); }
    MapViewOptions rg2; rg2.load(ini); check(rg2.grid == 0, "bad grid rejected");
    check(grid_lines(MapGrid::None).empty(), "no grid lines");
    check(grid_lines(MapGrid::Tropics).size() == 5 && grid_lines(MapGrid::Tropics)[3].lat1 > 23.43 && grid_lines(MapGrid::Tropics)[3].lat1 < 23.44, "tropic of Cancer");
    check(grid_lines(MapGrid::LatLng).size() == 5 + 13, "lat/lng line count");
    check(grid_lines(MapGrid::Maidenhead).size() == 17 + 19, "maidenhead field boundaries");
    MapScene gs; gs.sun.reset();
    const auto plain = render(gs);
    gs.grid = MapGrid::Maidenhead;
    check(render(gs) != plain, "grid changes the render");

    // Night shade off removes the shading from the render.
    MapScene sc; sc.sun = SubsolarPoint{20.0, 0.0}; sc.night_shade = true;
    const auto on = render(sc);
    sc.night_shade = false;
    const auto off = render(sc);
    check(on != off, "night shade toggles the render");
    return failures == 0 ? 0 : 1;
}
