#include "../src/qt/de_clock_manual_civil_adapter.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

static_assert(!std::is_same_v<hamclock::DeClockShiftedCivilSeconds, hamclock::UtcSeconds>);
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
void invariants(const hamclock::DeClockManualCivilResult& result) {
    using namespace hamclock;
    const bool ready = result.status == DeClockManualCivilStatus::Ready;
    check(result.civil.has_value() == ready, "civil fields present exactly when ready");
    check(result.manual_offset.has_value() == (result.desired_mode == DeClockMode::ManualOffset), "manual offset provenance matches requested mode");
    check(result.shifted_calendar.has_value() == (ready || result.status == DeClockManualCivilStatus::DisplayOutOfRange ||
          result.status == DeClockManualCivilStatus::InvalidConversion), "shifted calendar present exactly after representable addition");
}
} // namespace
int main() {
    using namespace hamclock;
    const auto lowest = std::numeric_limits<std::int64_t>::min();
    const auto highest = std::numeric_limits<std::int64_t>::max();
    const DeClockOffsetPolicy wide{{lowest},{highest}};
    const DeClockConfiguration automatic{DeClockMode::Automatic, std::nullopt};
    const DeClockConfiguration zero{DeClockMode::ManualOffset, UtcOffsetSeconds{0}};
    for (const auto& configuration : {automatic, zero}) {
        const auto missing = adapt_de_clock_manual_civil(configuration, wide, std::nullopt);
        check(missing.status == DeClockManualCivilStatus::MissingSource && !missing.actual_source,
              "missing source precedence preserves mode without fabricating epoch zero");
        invariants(missing);
    }
    const auto unresolved = adapt_de_clock_manual_civil(automatic, wide, UtcSeconds{0});
    check(unresolved.status == DeClockManualCivilStatus::AutomaticUnresolved && unresolved.actual_source->value == 0,
          "present epoch zero automatic remains unresolved without manual UTC fallback");
    invariants(unresolved);
    check(adapt_de_clock_manual_civil(automatic, wide, UtcSeconds{highest}).status == DeClockManualCivilStatus::AutomaticUnresolved,
          "automatic mode precedence never attempts range or Qt conversion");
    const struct { std::int64_t source, offset; BigClockCivilTime civil; } fixtures[] = {
        {0,0,{1970,1,1,0,0,0}}, {0,1,{1970,1,1,0,0,1}}, {0,-1,{1969,12,31,23,59,59}},
        {0,20700,{1970,1,1,5,45,0}}, {0,-12600,{1969,12,31,20,30,0}},
        {86399,1,{1970,1,2,0,0,0}}, {86400,-1,{1970,1,1,23,59,59}},
        {951782399,1,{2000,2,29,0,0,0}}, {951868800,-1,{2000,2,29,23,59,59}},
        {4107542399,1,{2100,3,1,0,0,0}}, {253402300799,-1,{9999,12,31,23,59,58}},
        {-62135596800,0,{1,1,1,0,0,0}}, {253402300799,0,{9999,12,31,23,59,59}},
        {-62135596799,-1,{1,1,1,0,0,0}}, {253402300798,1,{9999,12,31,23,59,59}}
    };
    for (const auto& fixture : fixtures) {
        const DeClockConfiguration configuration{DeClockMode::ManualOffset, UtcOffsetSeconds{fixture.offset}};
        const auto result = adapt_de_clock_manual_civil(configuration, wide, UtcSeconds{fixture.source});
        check(result.status == DeClockManualCivilStatus::Ready && result.civil && same(*result.civil, fixture.civil),
              "independent manual civil fixture with quarter-hour second date leap and product endpoints");
        check(result.actual_source && result.actual_source->value == fixture.source && result.manual_offset->value == fixture.offset &&
              result.shifted_calendar->value == fixture.source + fixture.offset,
              "actual source and manual offset retained independently of shifted calendar encoding");
        invariants(result);
    }
    for (auto source : {lowest, highest, kBigClockUtcCivilMinimumSeconds - 1, kBigClockUtcCivilMaximumSeconds + 1}) {
        const auto result = adapt_de_clock_manual_civil(zero, wide, UtcSeconds{source});
        check(result.status == DeClockManualCivilStatus::SourceOutOfRange && result.actual_source->value == source,
              "actual out of product bounds rejected before addition or Qt");
        invariants(result);
    }
    check(adapt_de_clock_manual_civil({DeClockMode::ManualOffset,UtcOffsetSeconds{1}}, wide,
          UtcSeconds{kBigClockUtcCivilMinimumSeconds - 1}).status == DeClockManualCivilStatus::SourceOutOfRange &&
          adapt_de_clock_manual_civil({DeClockMode::ManualOffset,UtcOffsetSeconds{-1}}, wide,
          UtcSeconds{kBigClockUtcCivilMaximumSeconds + 1}).status == DeClockManualCivilStatus::SourceOutOfRange,
          "actual source product bounds enforced even when offset would bring displayed civil into range");
    const struct { std::int64_t source, offset; DeClockManualCivilStatus status; std::int64_t shifted; } boundaries[] = {
        {1,highest,DeClockManualCivilStatus::OffsetOverflow,0},
        {-1,lowest,DeClockManualCivilStatus::OffsetOverflow,0},
        {0,highest,DeClockManualCivilStatus::DisplayOutOfRange,highest},
        {0,lowest,DeClockManualCivilStatus::DisplayOutOfRange,lowest},
        {-1,highest,DeClockManualCivilStatus::DisplayOutOfRange,highest-1},
        {1,lowest,DeClockManualCivilStatus::DisplayOutOfRange,lowest+1},
        {kBigClockUtcCivilMinimumSeconds,-1,DeClockManualCivilStatus::DisplayOutOfRange,kBigClockUtcCivilMinimumSeconds-1},
        {kBigClockUtcCivilMaximumSeconds,1,DeClockManualCivilStatus::DisplayOutOfRange,kBigClockUtcCivilMaximumSeconds+1}
    };
    for (const auto& boundary : boundaries) {
        const auto result = adapt_de_clock_manual_civil({DeClockMode::ManualOffset,UtcOffsetSeconds{boundary.offset}},
                                                       wide, UtcSeconds{boundary.source});
        check(result.status == boundary.status && result.actual_source->value == boundary.source &&
              result.manual_offset->value == boundary.offset, "overflow versus representable display range distinction preserves provenance");
        if (boundary.status == DeClockManualCivilStatus::DisplayOutOfRange) {
            check(result.shifted_calendar && result.shifted_calendar->value == boundary.shifted,
                  "representable signed64 extreme shifted encoding retained with distinct unit");
        }
        invariants(result);
    }
    for (const auto& actual : {std::optional<UtcSeconds>{}, std::optional<UtcSeconds>{UtcSeconds{0}}}) {
        for (int bad_mode : {std::numeric_limits<int>::min(), -1, 2, 255, std::numeric_limits<int>::max()}) {
            bool threw = false;
            try { adapt_de_clock_manual_civil({static_cast<DeClockMode>(bad_mode),std::nullopt}, wide, actual); }
            catch (const std::invalid_argument&) { threw = true; }
            check(threw, "full known-mode validation occurs before missing source early return");
        }
        for (const auto& invalid : {DeClockConfiguration{DeClockMode::Automatic,UtcOffsetSeconds{0}},
                                   DeClockConfiguration{DeClockMode::ManualOffset,std::nullopt}}) {
            bool threw = false;
            try { adapt_de_clock_manual_civil(invalid, wide, actual); }
            catch (const std::invalid_argument&) { threw = true; }
            check(threw, "complete mode payload validation occurs before unavailable source branch");
        }
        bool threw = false;
        try { adapt_de_clock_manual_civil(automatic, {{1},{-1}}, actual); }
        catch (const std::invalid_argument&) { threw = true; }
        check(threw, "caller bounds validated even for Automatic or missing source");
        threw = false;
        try { adapt_de_clock_manual_civil({DeClockMode::ManualOffset,UtcOffsetSeconds{2}}, {{-1},{1}}, actual); }
        catch (const std::invalid_argument&) { threw = true; }
        check(threw, "outside explicit policy rejected before early missing source return");
    }
    const DeClockConfiguration original{DeClockMode::ManualOffset,UtcOffsetSeconds{3600}};
    const UtcSeconds actual{0};
    auto copy = adapt_de_clock_manual_civil(original, wide, actual);
    copy.actual_source->value = 99;
    copy.manual_offset->value = 99;
    copy.civil->hour = 23;
    check(original.manual_offset->value == 3600 && actual.value == 0 &&
          adapt_de_clock_manual_civil(original, wide, actual).civil->hour == 1,
          "owned output mutation does not alter input or adapter state");
    std::cout << checks << " manual DE civil adapter checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
