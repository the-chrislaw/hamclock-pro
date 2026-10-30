// "Use my location" fix acceptance rules and DeviceLocation signal path.
#include "../src/platform/apple_devicelocation.hpp"
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
    check(judgeFix({51.5, -0.12, 50}) == FixVerdict::Accept, "good fix");
    check(judgeFix({-90, 180, 0}) == FixVerdict::Accept, "edges accepted");
    check(judgeFix({90.1, 0, 10}) == FixVerdict::BadCoordinate, "lat out of range");
    check(judgeFix({0, -180.5, 10}) == FixVerdict::BadCoordinate, "lon out of range");
    check(judgeFix({NAN, 0, 10}) == FixVerdict::BadCoordinate, "nan lat");
    check(judgeFix({10, 10, -1}) == FixVerdict::Invalid, "negative accuracy is invalid");
    check(judgeFix({10, 10, NAN}) == FixVerdict::Invalid, "nan accuracy");
    check(judgeFix({10, 10, kMaxAccuracyM}) == FixVerdict::Accept, "limit accepted");
    check(judgeFix({10, 10, kMaxAccuracyM + 1}) == FixVerdict::TooCoarse, "coarse rejected");
    check(describeVerdict(FixVerdict::Accept).isEmpty(), "accept has no message");
    check(!describeVerdict(FixVerdict::TooCoarse).isEmpty(), "coarse has a message");

    DeviceLocation loc;
    check(!loc.available(), "unavailable off Apple");
    int ok = 0, bad = 0;
    double glat = 0, glon = 0;
    QObject::connect(&loc, &DeviceLocation::located, [&](double a, double o) { ++ok; glat = a; glon = o; });
    QObject::connect(&loc, &DeviceLocation::failed, [&](const QString&) { ++bad; });
    loc.deliver({40.7, -74.0, 30});
    check(ok == 1 && bad == 0 && glat == 40.7 && glon == -74.0, "good fix emits located once");
    loc.deliver({40.7, -74.0, 50000});
    check(ok == 1 && bad == 1, "coarse fix emits failed");
    loc.fail(QStringLiteral("denied"));
    check(bad == 2, "native failure forwarded");
    return failures ? 1 : 0;
}
