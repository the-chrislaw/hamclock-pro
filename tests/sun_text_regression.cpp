#include "ui/sun_text.hpp"

#include <cstdio>

using namespace hamclock;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

int main() {
    CHECK(sun_countdown(0) == "in 0m");
    CHECK(sun_countdown(-5) == "in 0m");
    CHECK(sun_countdown(42 * 60) == "in 42m");
    CHECK(sun_countdown(3 * 3600 + 12 * 60) == "in 3h 12m");
    CHECK(sun_countdown(26 * 3600) == "in 1d 2h");

    // Greenwich at midnight UTC on the March equinox: sun rises about 06:00, sets about 18:00.
    const std::int64_t midnight = 1773964800; // 2026-03-20 00:00 UTC
    const QString g = sun_line(midnight, 51.48, 0.0);
    CHECK(g.startsWith("Rise in 5h") || g.startsWith("Rise in 6h"));
    CHECK(g.contains(" · Set in 17h") || g.contains(" · Set in 18h"));
    // Soonest first: at 12:00 UTC the set comes before the next rise.
    CHECK(sun_line(midnight + 12 * 3600, 51.48, 0.0).startsWith("Set in"));

    // Svalbard in midsummer / midwinter.
    CHECK(sun_line(1782086400, 78.2, 15.6) == "Sun up all day");    // 2026-06-22
    CHECK(sun_line(1797984000, 78.2, 15.6) == "Sun down all day");  // 2026-12-23
    CHECK(sun_line(midnight, 95.0, 0.0).isEmpty());
    return failures ? 1 : 0;
}
