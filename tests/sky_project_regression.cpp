// Camera AR projection: centre, offsets, behind-camera, roll, zenith, track skipping.
#include "../src/platform/apple_skyproject.hpp"
#include <QCoreApplication>
#include <cmath>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    const QSizeF screen(1000, 500);

    CameraView v;
    v.az_deg = 90; v.el_deg = 30; v.hfov_deg = 60; v.vfov_deg = 40;

    auto c = projectToScreen(v, 90, 30, screen);
    check(c.in_front && c.on_screen && near(c.pos.x(), 500) && near(c.pos.y(), 250), "target at centre");

    auto r = projectToScreen(v, 90 + 20, 30, screen);
    check(r.on_screen && r.pos.x() > 500 && near(r.pos.y(), 250, 40), "east of view axis is right of centre");
    auto l = projectToScreen(v, 90 - 20, 30, screen);
    check(l.on_screen && l.pos.x() < 500, "west of view axis is left of centre");
    check(near(r.pos.x() - 500, 500 - l.pos.x(), 1e-6), "left/right symmetric");

    auto up = projectToScreen(v, 90, 45, screen);
    check(up.on_screen && up.pos.y() < 250 && near(up.pos.x(), 500), "higher elevation is up the screen");
    auto dn = projectToScreen(v, 90, 15, screen);
    check(dn.on_screen && dn.pos.y() > 250, "lower elevation is down the screen");

    // Half the horizontal FOV along the horizon-level view lands on the edge.
    CameraView h; h.az_deg = 0; h.el_deg = 0; h.hfov_deg = 60; h.vfov_deg = 40;
    auto edge = projectToScreen(h, 29.9, 0, screen);
    check(edge.on_screen && edge.pos.x() > 990, "just inside the right edge");
    auto out = projectToScreen(h, 35, 0, screen);
    check(out.in_front && !out.on_screen && near(out.edge_angle_deg, 90, 3), "off the right edge points right");

    auto behind = projectToScreen(h, 200, 0, screen);
    check(!behind.in_front && !behind.on_screen && near(behind.edge_angle_deg, 270, 1e-6), "behind camera");

    // Looking at the zenith: a target at the horizon is behind the view plane.
    CameraView z; z.az_deg = 0; z.el_deg = 90;
    check(projectToScreen(z, 0, 90, screen).on_screen, "zenith at centre");
    check(!projectToScreen(z, 0, 0, screen).in_front, "horizon not in front when looking at zenith");

    // Roll 90 deg clockwise: an object to the right of the view moves to the bottom... scene turns CCW,
    // so right (east) goes up.
    CameraView rl = h; rl.roll_deg = 90;
    auto rolled = projectToScreen(rl, 10, 0, screen);
    check(rolled.in_front && rolled.pos.y() < 250 && near(rolled.pos.x(), 500, 1e-6), "roll turns scene counter-clockwise");

    // Invalid inputs.
    check(!projectToScreen(h, 0, 0, QSizeF(0, 0)).in_front, "empty screen");
    CameraView bad = h; bad.hfov_deg = 0;
    check(!projectToScreen(bad, 0, 0, screen).in_front, "zero fov");

    QVector<QPointF> track{{0, 0}, {10, 5}, {180, 0}, {-10, 5}};
    auto pts = projectTrack(h, track, screen);
    check(pts.size() == 3, "track skips points behind the camera");

    return failures ? 1 : 0;
}
