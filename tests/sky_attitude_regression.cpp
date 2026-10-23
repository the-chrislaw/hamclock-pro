// Device attitude -> az/el maths and SkySensor feed path (no sensors on Linux).
// Rows of the rotation matrix are the device x/y/z axes written in the
// reference frame (x north, y west, z up).
#include "../src/platform/apple_skysensor.hpp"
#include <QCoreApplication>
#include <vector>
#include <cmath>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }

hamclock::RotationMatrix rows(double x1, double x2, double x3, double y1, double y2, double y3,
                              double z1, double z2, double z3) {
    hamclock::RotationMatrix r;
    r.m11 = x1; r.m12 = x2; r.m13 = x3;
    r.m21 = y1; r.m22 = y2; r.m23 = y3;
    r.m31 = z1; r.m32 = z2; r.m33 = z3;
    return r;
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;

    // Identity: flat on a table, device x north, device y west.
    RotationMatrix flat;
    AzEl a = attitudeToAzEl(flat, PointingAxis::BackCamera);
    check(near(a.el, -90), "flat: back camera looks straight down");
    a = attitudeToAzEl(flat, PointingAxis::TopEdge);
    check(near(a.el, 0) && near(a.az, 270), "flat: top edge points west");

    // Upright, rear camera facing north.
    RotationMatrix north = rows(0, -1, 0, 0, 0, 1, -1, 0, 0);
    a = attitudeToAzEl(north, PointingAxis::BackCamera);
    check(near(a.az, 0) && near(a.el, 0), "upright north");
    // Upright, rear camera facing east.
    RotationMatrix east = rows(-1, 0, 0, 0, 0, 1, 0, 1, 0);
    a = attitudeToAzEl(east, PointingAxis::BackCamera);
    check(near(a.az, 90) && near(a.el, 0), "upright east");
    // Facing north tilted up 30 degrees.
    const double s = std::sin(M_PI / 6), c = std::cos(M_PI / 6);
    RotationMatrix up30 = rows(0, -1, 0, -s, 0, c, -c, 0, -s);
    a = attitudeToAzEl(up30, PointingAxis::BackCamera);
    check(near(a.az, 0) && near(a.el, 30), "north, tilted up 30");
    // Top edge of that same phone points 60 above the horizon, due north.
    a = attitudeToAzEl(up30, PointingAxis::TopEdge);
    check(near(a.az, 180) && near(a.el, 60), "top edge of tilted phone (behind the tilt)");

    // Straight up: azimuth undefined but finite.
    RotationMatrix zenith = rows(0, -1, 0, -1, 0, 0, 0, 0, -1);
    a = attitudeToAzEl(zenith, PointingAxis::BackCamera);
    check(near(a.el, -90) || near(a.el, 90), "vertical elevation");

    // Sensor: unavailable on Linux, but the feed path smooths and emits.
    SkySensor sensor;
    check(!sensor.available(), "no sensor on Linux");
    sensor.start();
    check(!sensor.running(), "start is a no-op without a sensor");
    std::vector<std::pair<double, double>> spy;
    QObject::connect(&sensor, &SkySensor::attitudeChanged, [&](double az, double el) { spy.emplace_back(az, el); });
    sensor.feedAttitude(north);
    check(spy.size() == 1 && near(spy[0].first, 0) && near(spy[0].second, 0),
          "first sample passes through");
    sensor.feedAttitude(east);
    check(spy.size() == 2 && spy[1].first > 0 && spy[1].first < 90,
          "second sample is smoothed");

    std::cout << (failures ? "FAILED\n" : "OK\n");
    return failures ? 1 : 0;
}
