#include "../src/qt/de_time_zone.hpp"
#include <iostream>

namespace {
int failures = 0;
void check(bool okay, const char* label) {
    if (!okay) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace
int main() {
    using namespace hamclock;
    // Reference instants: US DST began 2026-03-08 07:00:00Z (2am EST -> 3am EDT)
    // and ends 2026-11-01 06:00:00Z (2am EDT -> 1am EST), per US federal rules
    // (Energy Policy Act of 2005: second Sunday of March, first Sunday of November).
    const std::int64_t spring = 1772953200; // 2026-03-08T07:00:00Z
    const std::int64_t autumn = 1793512800; // 2026-11-01T06:00:00Z
    const char* ny = "America/New_York";
    auto offset = [&](const char* id, std::int64_t t) { return resolve_de_zone_offset(id, UtcSeconds{t}); };
    check(offset(ny, spring - 1) && offset(ny, spring - 1)->value == -5 * 3600, "New York is EST just before spring forward");
    check(offset(ny, spring) && offset(ny, spring)->value == -4 * 3600, "New York is EDT at spring forward");
    check(offset(ny, autumn - 1) && offset(ny, autumn - 1)->value == -4 * 3600, "New York is EDT just before fall back");
    check(offset(ny, autumn) && offset(ny, autumn)->value == -5 * 3600, "New York is EST at fall back");
    // Southern hemisphere and a half-hour zone without DST.
    check(offset("Australia/Sydney", spring) && offset("Australia/Sydney", spring)->value == 11 * 3600, "Sydney is AEDT in March");
    check(offset("Asia/Kolkata", spring) && offset("Asia/Kolkata", spring)->value == 19800, "Kolkata is UTC+5:30");
    check(offset("UTC", spring) && offset("UTC", spring)->value == 0, "UTC zone has zero offset");
    check(!offset("Not/AZone", spring), "unknown zone id yields no offset");
    check(!offset("", spring), "empty zone id yields no offset");
    check(is_valid_de_zone_id("Europe/London") && !is_valid_de_zone_id("Mars/Olympus"), "zone id validation");
    if (failures) return 1;
    std::cout << "de time zone checks passed\n";
}
