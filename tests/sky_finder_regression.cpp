// SkyFinder: guidance, single haptic tick per lock, hysteresis, stop resets.
#include "../src/platform/apple_skyfinder.hpp"
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

    SkySensor sensor;
    SkyFinder finder(&sensor);
    int ticks = 0, locks = 0;
    finder.setHaptic([&] { ++ticks; });
    QObject::connect(&finder, &SkyFinder::lockedOn, [&] { ++locks; });

    finder.setTarget(90, 30);
    emit sensor.attitudeChanged(50, 18);
    check(finder.guidance().isEmpty(), "ignored before start");
    finder.start(); // sensors are absent on Linux; the finder still arms itself
    check(finder.active(), "start arms the finder");
    emit sensor.attitudeChanged(50, 18);
    check(finder.guidance().contains("right") && finder.guidance().contains("up"), "guides right and up");
    check(!finder.locked() && ticks == 0, "not locked far away");

    emit sensor.attitudeChanged(89, 29);
    check(finder.locked() && ticks == 1 && locks == 1, "lock fires one tick");
    emit sensor.attitudeChanged(90, 30);
    emit sensor.attitudeChanged(88, 31);
    check(ticks == 1, "no chatter while locked");
    emit sensor.attitudeChanged(60, 30);
    check(!finder.locked(), "leaves lock beyond exit angle");
    emit sensor.attitudeChanged(90, 30);
    check(ticks == 2, "re-arrival ticks again");

    finder.stop();
    check(!finder.active() && finder.guidance().isEmpty() && !finder.locked(), "stop clears guidance");
    emit sensor.attitudeChanged(90, 30);
    check(ticks == 2, "ignored after stop");

    std::cout << (failures ? "FAILED\n" : "sky-finder-regression: ok\n");
    return failures ? 1 : 0;
}
