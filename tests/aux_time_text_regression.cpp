#include "ui/aux_time.hpp"

#include <cstdio>

using namespace hamclock;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

int main() {
    // 2000-01-01 12:00:00 UTC = JD 2451545.0, MJD 51544.5, UNIX 946728000.
    const qint64 j2000 = 946728000;
    CHECK(aux_time_text(1, j2000, 0) == "JD 2451545.00000");
    CHECK(aux_time_text(2, j2000, 0) == "MJD 51544.50000");
    CHECK(aux_time_text(4, j2000, 0) == "UNIX 946728000");
    CHECK(aux_time_text(0, j2000, 0) == "Day 1 of 366 · week 52");
    // GMST at J2000 is 18h 41m 50.5s (Meeus); east longitude 15 deg adds one hour.
    CHECK(aux_time_text(3, j2000, 0) == "LST 18:41:50");
    CHECK(aux_time_text(3, j2000, 15.0) == "LST 19:41:50");
    CHECK(aux_time_text(3, j2000, -90.0) == "LST 12:41:50");
    CHECK(aux_time_text(5, j2000, 0) == aux_time_text(0, j2000, 0));
    CHECK(aux_time_text(-1, j2000, 0) == aux_time_text(4, j2000, 0));
    CHECK(next_aux_time_mode(4) == 0 && next_aux_time_mode(0) == 1);
    const double lst = local_sidereal_hours(j2000, -179.0);
    CHECK(lst >= 0.0 && lst < 24.0);
    return failures ? 1 : 0;
}
