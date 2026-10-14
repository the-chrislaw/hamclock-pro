#include <cmath>
#include "../src/map/view_options.hpp"
#include "../src/map/map_renderer.hpp"
#include "../src/map/sky_points.hpp"
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
    o.center_lon = -90; check(o.save(ini), "save centre");
    MapViewOptions rc; rc.load(ini); check(rc.center_lon == -90, "centre round trip");
    { QFile g(ini); g.open(QIODevice::WriteOnly | QIODevice::Append); g.write("\n[map]\ncenter=500\n"); }
    MapViewOptions rc2; rc2.load(ini); check(rc2.center_lon == MapViewOptions::center_auto, "bad centre rejected");
    { MapScene cs; cs.de = Location{40, -75};
      const auto vde = MapRenderer::viewportFor(QSizeF(1024, 600), cs);
      check(std::abs(vde.center_lon + 75) < 1e-9, "centre follows DE by default");
      const auto vw = [&] { auto w = cs; w.center_lon = 90; return MapRenderer::viewportFor(QSizeF(800, 310), w); }();
      check(std::abs(vw.center_lon - 90) < 1e-9 && std::abs(vw.x_of(90) - 400) < 1e-6, "fixed centre applies to a card that shows the whole world");
      cs.center_lon = 90;
      const auto vc = MapRenderer::viewportFor(QSizeF(1024, 600), cs);
      check(std::abs(vc.center_lon - 90) < 1e-9, "fixed centre overrides DE");
      check(std::abs(vc.x_of(90) - 512) < 1e-6, "fixed centre places meridian mid-card"); }
    check(grid_lines(MapGrid::None).empty(), "no grid lines");
    check(grid_lines(MapGrid::Tropics).size() == 5 && grid_lines(MapGrid::Tropics)[3].lat1 > 23.43 && grid_lines(MapGrid::Tropics)[3].lat1 < 23.44, "tropic of Cancer");
    check(grid_lines(MapGrid::LatLng).size() == 5 + 13, "lat/lng line count");
    check(grid_lines(MapGrid::Maidenhead).size() == 17 + 19, "maidenhead field boundaries");
    MapScene gs; gs.sun.reset();
    const auto plain = render(gs);
    gs.grid = MapGrid::Maidenhead;
    check(render(gs) != plain, "grid changes the render");

    // Azimuth rings: every point is the requested arc distance from DE, drawn only with DE.
    for (const auto& pt : ring_points(40.0, -75.0, 60.0)) {
        const double r = 3.14159265358979323846 / 180.0;
        const double c = std::sin(40 * r) * std::sin(pt.lat * r) + std::cos(40 * r) * std::cos(pt.lat * r) * std::cos((pt.lon + 75.0) * r);
        check(std::fabs(std::acos(std::fmax(-1.0, std::fmin(1.0, c))) / r - 60.0) < 1e-6, "ring point at 60 deg from DE");
    }
    check(ring_points(0, 0, 90).size() == 121 && std::fabs(ring_points(10, 20, 30).front().lat - ring_points(10, 20, 30).back().lat) < 1e-9 , "ring sampled");
    check(grid_lines(MapGrid::AzimuthRings).empty(), "rings are not graticule lines");
    gs.grid = MapGrid::AzimuthRings;
    const auto norings = render(gs);
    gs.de = Location{40.0, -75.0};
    check(render(gs) != norings, "azimuth rings drawn around DE");
    // Sky points: sublunar point and antipode.
    check(antipode({40, -75}).latitude == -40 && std::fabs(antipode({40, -75}).longitude - 105) < 1e-9, "antipode");
    check(std::fabs(antipode({0, 0}).longitude + 180) < 1e-9, "antipode of 0,0 wraps to -180");
    check(std::fabs(greenwich_sidereal_deg({946728000}) - 280.46061837) < 1e-6, "GMST at J2000.0");
    { const auto sl = sublunar_point({1760400000});
      check(std::fabs(sl.latitude_deg) < 29.0 && sl.longitude_deg >= -180 && sl.longitude_deg < 180, "sublunar point in range");
      // Moon overhead at its own sublunar point: topocentric elevation near 90 degrees.
      const auto h = moon_horizontal({1760400000}, sl.latitude_deg, sl.longitude_deg);
      check(h.elevation_deg > 87.0, "Moon near zenith at sublunar point"); }
    { MapScene ss; ss.sun.reset(); ss.de = Location{40, -75}; ss.moon = Location{10, 20};
      const auto base = render(ss); ss.sky_points = true;
      check(render(ss) != base, "sky points drawn"); }
    { MapViewOptions sp; sp.sky_points = true; check(sp.save(ini), "save sky"); MapViewOptions r; r.load(ini); check(r.sky_points, "sky points round trip"); }
    // Night shade off removes the shading from the render.
    MapScene sc; sc.sun = SubsolarPoint{20.0, 0.0}; sc.night_shade = true;
    const auto on = render(sc);
    sc.night_shade = false;
    const auto off = render(sc);
    check(on != off, "night shade toggles the render");
    return failures == 0 ? 0 : 1;
}
