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
    return failures ? 1 : 0;
}
