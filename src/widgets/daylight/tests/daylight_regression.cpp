// Yearly chart data: London solstice rows against almanac sunrise/sunset (UTC),
// polar gaps at Tromso, and the sampling step.
#include "../daylight_model.hpp"
#include <QCoreApplication>
#include <QVariantMap>
#include <cmath>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    // London (51.5N, 0.13W) 2026: 21 June (day 172) rises ~03:43 UTC, sets ~20:21 UTC;
    // 21 December (day 355) rises ~08:04, sets ~15:53 UTC.
    const auto pts = daylight_points(2026, 51.5, -0.13, 1);
    check(pts.size() == 365, "daily points");
    const auto june = pts[171].toMap();
    check(june["day"].toInt() == 172, "day numbering");
    check(std::fabs(june["rise"].toDouble() - (3 * 60 + 43)) < 4, "June sunrise");
    check(std::fabs(june["set"].toDouble() - (20 * 60 + 21)) < 4, "June sunset");
    const auto dec = pts[354].toMap();
    check(std::fabs(dec["rise"].toDouble() - (8 * 60 + 4)) < 4, "December sunrise");
    check(std::fabs(dec["set"].toDouble() - (15 * 60 + 53)) < 4, "December sunset");
    check(daylight_points(2026, 51.5, -0.13, 7).size() == 53, "weekly sampling");
    check(daylight_points(2026, 51.5, -0.13, 0).size() == 365, "step clamps to 1");
    const auto tromso = daylight_points(2026, 69.65, 18.96, 1);
    check(tromso[171].toMap()["rise"].toDouble() < 0, "midnight sun has no rise");
    check(tromso[354].toMap()["set"].toDouble() < 0, "polar night has no set");
    // New York (40.7N, 74.0W) 21 June 2026: sunrise ~05:25 EDT, sunset ~20:31 EDT; 21 Dec EST ~07:16 / 16:32.
    const auto ny = daylight_points(2026, 40.71, -74.0, 1, "America/New_York");
    check(std::fabs(ny[171].toMap()["rise"].toDouble() - (5 * 60 + 25)) < 5, "NY June sunrise local (EDT)");
    check(std::fabs(ny[171].toMap()["set"].toDouble() - (20 * 60 + 31)) < 5, "NY June sunset local (EDT)");
    check(std::fabs(ny[354].toMap()["rise"].toDouble() - (7 * 60 + 16)) < 5, "NY Dec sunrise local (EST)");
    check(std::fabs(ny[354].toMap()["set"].toDouble() - (16 * 60 + 32)) < 5, "NY Dec sunset local (EST)");
    check(daylight_points(2026, 40.71, -74.0, 1, "Not/AZone")[171].toMap()["rise"].toDouble() > 8 * 60, "bad zone stays UTC");
    check(tromso[171].toMap()["rise"].toDouble() < 0 && daylight_points(2026, 69.65, 18.96, 1, "Europe/Oslo")[171].toMap()["rise"].toDouble() < 0, "polar gap survives zone shift");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
