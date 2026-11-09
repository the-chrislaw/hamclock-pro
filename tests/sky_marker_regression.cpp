// SkyMarker: target + attitude + preview become a screen position.
#include "../src/platform/apple_skymarker.hpp"
#include <QCoreApplication>
#include <cmath>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    SkySensor sensor;
    SkyFinder finder(&sensor);
    SkyMarker marker(&sensor, &finder);
    marker.setPreview(400, 800, 40, 70);
    check(!marker.inFront() && !marker.visible(), "nothing without target or attitude");

    finder.setTarget(90, 30);
    emit sensor.attitudeChanged(90, 30);
    check(marker.visible(), "target on the view axis is visible");
    check(std::fabs(marker.x() - 200) < 1 && std::fabs(marker.y() - 400) < 1, "on-axis target is at the centre");

    int changes = 0;
    QObject::connect(&marker, &SkyMarker::changed, [&] { ++changes; });
    emit sensor.attitudeChanged(90, 30);
    check(changes == 0, "no change signal when nothing moved");
    emit sensor.attitudeChanged(100, 30);
    check(changes == 1 && marker.x() < 200, "turning right moves the marker left");

    emit sensor.attitudeChanged(270, 30);
    check(!marker.visible(), "target behind the phone is not visible");

    emit sensor.attitudeChanged(90, 30);
    finder.clearTarget();
    check(!marker.visible() && !marker.inFront(), "cleared target hides the marker");

    // Pass track: only on-screen, in-front samples; a break marks a gap.
    emit sensor.attitudeChanged(90, 30);
    marker.setTrack({{90, 30}, {92, 32}, {200, 30}, {88, 28}});
    const QVariantList tp = marker.trackPoints();
    check(tp.size() == 3, "off-screen/behind track samples are dropped");
    check(tp.size() == 3 && !tp[0].toMap()["gap"].toBool() && !tp[1].toMap()["gap"].toBool() &&
              tp[2].toMap()["gap"].toBool(),
          "a break in the track is flagged on the next point");
    emit sensor.attitudeChanged(270, 30);
    check(marker.trackPoints().size() == 0, "track behind the phone draws nothing");
    marker.setTrack({});
    check(marker.trackPoints().isEmpty(), "empty track clears");

    // Roll: phone looking north at 30 deg elevation, turned clockwise 90 degrees
    // (landscape). A target above the view axis then appears to the left.
    {
        SkySensor s2;
        s2.setAvailableOverride(true);
        SkyFinder f2(&s2);
        SkyMarker m2(&s2, &f2);
        m2.setPreview(400, 800, 40, 70);
        const double el = 30 * M_PI / 180, roll = 90 * M_PI / 180;
        // f = (cos el, 0, sin el); u0 = (-sin el, 0, cos el); right r0 = f x u0 = (0, -1, 0)
        const double fx = std::cos(el), fz = std::sin(el), ox = -std::sin(el), oz = std::cos(el);
        const double ux = std::cos(roll) * ox, uy = -std::sin(roll), uz = std::cos(roll) * oz;
        // rows: device x (right) = f x u, device y = u, device z = -f
        RotationMatrix r;
        r.m11 = (0 * uz - fz * uy);
        r.m12 = (fz * ux - fx * uz);
        r.m13 = (fx * uy - 0 * ux);
        r.m21 = ux; r.m22 = uy; r.m23 = uz;
        r.m31 = -fx; r.m32 = 0; r.m33 = -fz;
        f2.setTarget(0, 35); // 5 degrees above the view axis
        s2.feedAttitude(r);
        check(m2.visible() && m2.x() < 200 && std::fabs(m2.y() - 400) < 60, "rolled 90 degrees clockwise: up appears to the left");
    }
    return failures ? 1 : 0;
}
