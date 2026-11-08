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
    return failures ? 1 : 0;
}
