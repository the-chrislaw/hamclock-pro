#include "ui/clock_text.hpp"

#include <cstdio>

using namespace hamclock;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

int main() {
    const QDateTime t(QDate(2026, 10, 14), QTime(23, 30, 5), Qt::UTC);
    CHECK(clock_time(t, 0) == "23:30:05");
    CHECK(clock_time(t, 3600) == "00:30:05");
    CHECK(clock_date(t, 3600) == "Thu 15 Oct 2026");
    CHECK(clock_date(t, -7 * 3600) == "Wed 14 Oct 2026");
    CHECK(clock_time(t, -7 * 3600 - 1800) == "16:00:05");
    CHECK(clock_time(t, 0, true) == "11:30:05 PM");
    CHECK(clock_time(t, 3600, true) == "12:30:05 AM");
    CHECK(clock_date(t, 3600, 1) == "Thu Oct 15 2026");
    CHECK(clock_date(t, 3600, 2) == "2026-10-15 Thu");
    CHECK(clock_date(t, 3600, 9) == "Thu 15 Oct 2026");
    CHECK(zone_local_text("Asia/Tokyo", t) == "Local 08:30 · Tokyo (auto)");
    CHECK(zone_local_text("America/Los_Angeles", t, true, false) == "Local 4:30 PM · Los Angeles");
    CHECK(zone_local_text("America/Argentina/Buenos_Aires", t, false, false) == "Local 20:30 · Buenos Aires");
    CHECK(zone_local_text("", t).isEmpty());
    CHECK(zone_local_text("Not/AZone", t).isEmpty());
    return failures ? 1 : 0;
}
