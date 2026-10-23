// Find-in-the-sky pointing maths: wrap, guidance text, circular smoothing, lock hysteresis.
#include "../src/platform/apple_skypointer.hpp"
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

    check(near(wrapSigned(190), -170) && near(wrapSigned(-190), 170) && near(wrapSigned(180), 180) &&
              near(wrapSigned(720), 0) && near(wrapSigned(-180), 180),
          "wrapSigned");

    auto g = pointingGuidance(350, 10, 10, 10);
    check(near(g.turn_deg, 20) && near(g.tilt_deg, 0), "turn right across north");
    check(g.text == QStringLiteral("Turn right 20°"), "text right only");
    g = pointingGuidance(90, 10, 50, 22);
    check(near(g.turn_deg, -40) && g.text == QStringLiteral("Turn left 40°, tilt up 12°"), "left and up");
    g = pointingGuidance(90, 40, 90, 20);
    check(g.text == QStringLiteral("Tilt down 20°"), "tilt down only, capitalised");
    g = pointingGuidance(100, 30, 101, 31);
    check(g.locked && g.text == QStringLiteral("On target"), "locked within 3 degrees");
    // Opposite azimuths near the zenith are only 2 degrees apart on the sky.
    g = pointingGuidance(0, 89, 180, 89);
    check(g.locked && g.off_axis_deg > 1.9 && g.off_axis_deg < 2.1, "off-axis shrinks near zenith");

    AttitudeSmoother s(0.5);
    check(!s.primed(), "unprimed");
    s.feed(358, 10);
    s.feed(2, 10);
    check(s.primed() && near(s.az(), 0, 1e-6), "smooths across the wrap");
    s.feed(2, 20);
    check(near(s.el(), 15), "elevation smoothed");
    s.reset();
    check(!s.primed(), "reset");

    LockTracker t;
    check(!t.update(10), "far: no tick");
    check(t.update(2.5) && t.locked(), "enter: one tick");
    check(!t.update(2.0) && !t.update(4.5) && t.locked(), "stays locked inside hysteresis");
    check(!t.update(6) && !t.locked(), "leaves above exit");
    check(t.update(1), "ticks again on re-entry");

    std::cout << (failures ? "FAILED\n" : "sky-pointer-regression: ok\n");
    return failures ? 1 : 0;
}
