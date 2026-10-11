#include "../src/qt/big_clock_civil_adapter.hpp"
#include <iostream>
#include <limits>

namespace {
int checks = 0, failures = 0;
void check(bool okay, const char* label) {
    ++checks;
    if (!okay) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool same(const hamclock::BigClockCivilTime& a, const hamclock::BigClockCivilTime& b) {
    return a.year == b.year && a.month == b.month && a.day == b.day &&
           a.hour == b.hour && a.minute == b.minute && a.second == b.second;
}
// Independent test oracle: count whole Gregorian years and explicit month
// lengths from year1, then shift to POSIX epoch. No Qt datetime oracle involved.
std::int64_t days_before_year(int year) {
    const auto y = static_cast<std::int64_t>(year - 1);
    return 365 * y + y / 4 - y / 100 + y / 400;
}
int days_in_month(int year, int month) {
    constexpr int lengths[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    return lengths[month - 1] + (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0) ? 1 : 0);
}
} // namespace
int main() {
    using namespace hamclock;
    const auto missing = adapt_big_clock_utc_civil(std::nullopt);
    check(missing.status == BigClockUtcCivilStatus::MissingSource && !missing.source && !missing.civil,
          "missing source is absence rather than fabricated epoch zero");
    const struct { std::int64_t seconds; BigClockCivilTime civil; } fixtures[] = {
        {-62135596800, {1,1,1,0,0,0}}, {-62135596799, {1,1,1,0,0,1}},
        {-1, {1969,12,31,23,59,59}}, {0, {1970,1,1,0,0,0}}, {1, {1970,1,1,0,0,1}},
        {951782399, {2000,2,28,23,59,59}}, {951782400, {2000,2,29,0,0,0}},
        {951868800, {2000,3,1,0,0,0}}, {4107542399, {2100,2,28,23,59,59}},
        {4107542400, {2100,3,1,0,0,0}}, {253402300798, {9999,12,31,23,59,58}},
        {253402300799, {9999,12,31,23,59,59}}
    };
    for (const auto& fixture : fixtures) {
        const UtcSeconds injected{fixture.seconds};
        const auto result = adapt_big_clock_utc_civil(injected);
        check(result.status == BigClockUtcCivilStatus::Ready && result.civil && same(*result.civil, fixture.civil),
              "independent literal UTC fixture including epoch negative boundary and century leap");
        check(result.source && result.source->value == injected.value && injected.value == fixture.seconds,
              "owned conversion retains exact injected POSIX seconds without caller mutation");
    }
    for (auto seconds : {std::numeric_limits<std::int64_t>::min(), kBigClockUtcCivilMinimumSeconds - 1,
                         kBigClockUtcCivilMaximumSeconds + 1, std::numeric_limits<std::int64_t>::max()}) {
        const auto result = adapt_big_clock_utc_civil(UtcSeconds{seconds});
        check(result.status == BigClockUtcCivilStatus::OutOfRange && result.source &&
              result.source->value == seconds && !result.civil, "extreme or just outside product bound typed rejection with no civil fallback");
    }
    const auto epoch_days = days_before_year(1970);
    for (int year : {1,4,100,400,1582,1600,1700,1800,1900,1969,1970,1999,2000,2004,2099,2100,2400,9999}) {
        auto day = days_before_year(year) - epoch_days;
        for (int month = 1; month <= 12; ++month) {
            const auto length = days_in_month(year, month);
            for (int date : {1, length}) {
                const auto seconds = (day + date - 1) * 86400 + 86399;
                const auto result = adapt_big_clock_utc_civil(UtcSeconds{seconds});
                check(result.status == BigClockUtcCivilStatus::Ready && result.civil &&
                      same(*result.civil, BigClockCivilTime{year,month,date,23,59,59}),
                      "independent Gregorian day-count month boundary sampling");
            }
            day += length;
        }
    }
    auto owned = adapt_big_clock_utc_civil(UtcSeconds{0});
    owned.civil->year = 9999;
    check(adapt_big_clock_utc_civil(UtcSeconds{0}).civil->year == 1970, "returned civil values are owned and cannot mutate adapter state");
    std::cout << checks << " UTC civil adapter checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
