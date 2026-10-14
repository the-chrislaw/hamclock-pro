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
    return failures ? 1 : 0;
}
