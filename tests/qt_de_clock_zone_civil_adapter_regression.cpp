#include "../src/qt/de_clock_zone_civil_adapter.hpp"
#include <iostream>

namespace {
int failures = 0;
void check(bool okay, const char* label) {
    if (!okay) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace
int main() {
    using namespace hamclock;
    // US DST: began 2026-03-08 07:00:00Z, ends 2026-11-01 06:00:00Z
    // (Energy Policy Act of 2005).
    const std::int64_t spring = 1772953200, autumn = 1793512800;
    auto ny = [&](std::int64_t t) { return adapt_de_clock_zone_civil("America/New_York", UtcSeconds{t}); };

    auto a = ny(spring - 1); // 01:59:59 EST
    check(a.status == DeClockZoneCivilStatus::Ready && a.civil && a.civil->hour == 1 && a.civil->minute == 59 &&
          a.civil->day == 8 && a.offset && a.offset->value == -5 * 3600, "New York 01:59:59 EST before spring forward");
    auto b = ny(spring); // 03:00:00 EDT
    check(b.status == DeClockZoneCivilStatus::Ready && b.civil && b.civil->hour == 3 && b.civil->minute == 0 &&
          b.offset && b.offset->value == -4 * 3600, "New York 03:00:00 EDT at spring forward");
    auto c = ny(autumn); // 01:00:00 EST (second 1am)
    check(c.status == DeClockZoneCivilStatus::Ready && c.civil && c.civil->hour == 1 && c.civil->day == 1 &&
          c.civil->month == 11 && c.offset && c.offset->value == -5 * 3600, "New York 01:00:00 EST at fall back");
    auto d = adapt_de_clock_zone_civil("Asia/Kolkata", UtcSeconds{spring}); // 12:30 on 2026-03-08
    check(d.status == DeClockZoneCivilStatus::Ready && d.civil && d.civil->hour == 12 && d.civil->minute == 30, "Kolkata +5:30");

    check(adapt_de_clock_zone_civil("Not/AZone", UtcSeconds{spring}).status == DeClockZoneCivilStatus::UnknownZone, "unknown zone");
    check(!adapt_de_clock_zone_civil("Not/AZone", UtcSeconds{spring}).civil, "unknown zone has no civil fields");
    check(adapt_de_clock_zone_civil("America/New_York", std::nullopt).status == DeClockZoneCivilStatus::MissingSource, "missing source");
    check(adapt_de_clock_zone_civil("America/New_York", UtcSeconds{INT64_MAX}).status == DeClockZoneCivilStatus::SourceOutOfRange, "source out of range");
    if (failures) return 1;
    std::cout << "de zone civil adapter checks passed\n";
}
