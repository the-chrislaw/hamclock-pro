#include "ui/alarms.hpp"

#include <cstdio>

using namespace hamclock;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

static QDateTime utc(int d, int h, int m, int s = 0) { return QDateTime(QDate(2026, 10, d), QTime(h, m, s), Qt::UTC); }

int main() {
    Alarm a;
    a.minute_of_day = 7 * 60 + 30;
    CHECK(alarm_time_text(a.minute_of_day) == "07:30 UTC");
    CHECK(alarm_time_text(-1) == "23:59 UTC");
    CHECK(alarm_next(a, utc(15, 6, 0)) == utc(15, 7, 30));
    CHECK(alarm_next(a, utc(15, 7, 30)) == utc(16, 7, 30));  // strictly after now
    CHECK(alarm_next(a, utc(15, 23, 59)) == utc(16, 7, 30));

    CHECK(!alarm_due(a, utc(15, 7, 29, 58), utc(15, 7, 29, 59)));
    CHECK(alarm_due(a, utc(15, 7, 29, 59), utc(15, 7, 30, 0)));   // inclusive end
    CHECK(!alarm_due(a, utc(15, 7, 30, 0), utc(15, 7, 30, 1)));   // exclusive start: no double ring
    CHECK(alarm_due(a, utc(15, 7, 29, 0), utc(15, 7, 31, 0)));    // a late tick still rings
    CHECK(!alarm_due(a, utc(15, 7, 0), utc(15, 9, 0)));           // 2 h jump is not elapsed time
    CHECK(!alarm_due(a, utc(15, 7, 30, 1), utc(15, 7, 29, 0)));   // clock set backwards
    CHECK(!alarm_due(a, QDateTime(), utc(15, 7, 30)));

    Alarm over;  // crosses midnight
    over.minute_of_day = 0;
    CHECK(alarm_due(over, utc(15, 23, 59, 59), utc(16, 0, 0, 1)));

    a.enabled = false;
    CHECK(!alarm_next(a, utc(15, 6, 0)).isValid());
    CHECK(!alarm_due(a, utc(15, 7, 29, 59), utc(15, 7, 30, 0)));
    CHECK(alarm_in_text(a, utc(15, 6, 0)) == "off");
    a.enabled = true;
    CHECK(alarm_in_text(a, utc(15, 6, 0)) == "in 1h 30m");
    CHECK(alarm_in_text(a, utc(15, 7, 10)) == "in 20m");
    CHECK(alarm_in_text(a, utc(15, 7, 29, 30)) == "in 1m");
    CHECK(alarm_in_text(a, utc(15, 7, 30, 0)) == "in 24h 0m");

    return failures ? 1 : 0;
}
