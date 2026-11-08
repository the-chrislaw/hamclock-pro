// SkyTargetBridge: selected-satellite detail becomes the finder target.
#include "sky_target_regression.hpp"
#include "../src/platform/apple_skytarget.hpp"
#include <QCoreApplication>
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
    double az = -1, el = -1;
    check(!finderTargetFromDetail({}, az, el), "empty detail rejected");
    check(!finderTargetFromDetail({{"az", 90}, {"el", -5}, {"above", false}}, az, el), "below horizon rejected");
    check(!finderTargetFromDetail({{"az", 400}, {"el", 10}, {"above", true}}, az, el), "bad azimuth rejected");
    check(finderTargetFromDetail({{"az", 120}, {"el", 35}, {"above", true}}, az, el) && az == 120 && el == 35, "valid target");

    SkySensor sensor;
    sensor.setAvailableOverride(true);
    SkyFinder finder(&sensor);
    FakeSource src;
    SkyTargetBridge bridge(&src, &finder);
    check(!bridge.hasTarget(), "no target initially");
    src.set({{"az", 90}, {"el", 30}, {"above", true}});
    check(bridge.hasTarget(), "target after detail change");
    finder.start();
    emit sensor.attitudeChanged(89, 29);
    check(finder.locked(), "finder uses the bridged target");
    src.set({{"az", 90}, {"el", -3}, {"above", false}});
    check(!bridge.hasTarget(), "target dropped once it sets");
    check(!finder.locked(), "finder lock cleared once target sets");
    check(finder.guidance().isEmpty(), "finder guidance cleared once target sets");
    emit sensor.attitudeChanged(90, 30);
    check(!finder.locked(), "no guidance without a target");

    check(trackFromSkyTrack({}).empty(), "empty skyTrack");
    check(trackFromSkyTrack({QVariantMap{{"az", 10}, {"el", 5}}, QVariantMap{{"el", 7}}, 3}).size() == 1, "malformed samples skipped");
    SkyMarker marker(&sensor, &finder);
    marker.setPreview(400, 800, 60, 100);
    SkyTrackBridge tb(&src, &marker);
    check(marker.trackPoints().isEmpty(), "no track initially");
    QVariantList pass;
    for (int i = 0; i < 5; ++i) pass.append(QVariantMap{{"az", 88 + i}, {"el", 28 + i}});
    src.setTrack(pass);
    emit sensor.attitudeChanged(90, 30);
    check(!marker.trackPoints().isEmpty(), "track points after the model publishes a pass");
    src.setTrack({});
    check(marker.trackPoints().isEmpty(), "track cleared with the model's track");
    return failures ? 1 : 0;
}
