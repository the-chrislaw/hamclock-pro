#include "../src/app/big_clock_presentation.hpp"

#include <array>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <type_traits>

namespace { std::size_t allocation_count = 0; }
void* operator new(std::size_t size) {
    ++allocation_count;
    if (void* result = std::malloc(size == 0 ? 1 : size)) return result;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

namespace {
using namespace hamclock;
int checks = 0;
int failures = 0;
void check(bool condition, const char* label) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
template<class Action> bool rejects(Action action) {
    try { action(); }
    catch (const std::invalid_argument&) { return true; }
    return false;
}
template<std::size_t Size>
bool text_equal(const std::array<char, Size>& actual, const char* expected) {
    bool end = false;
    for (std::size_t index = 0; index < Size; ++index) {
        const char character = end ? '\0' : expected[index];
        if (character == '\0') end = true;
        if (actual[index] != character) return false;
    }
    return end;
}
bool same_civil(const BigClockCivilTime& a, const BigClockCivilTime& b) {
    return a.year == b.year && a.month == b.month && a.day == b.day &&
           a.hour == b.hour && a.minute == b.minute && a.second == b.second;
}
bool same_optional(const std::optional<BigClockCivilTime>& a,
                   const std::optional<BigClockCivilTime>& b) {
    return a.has_value() == b.has_value() && (!a || same_civil(*a, *b));
}
bool same_times(const BigClockCivilTimes& a, const BigClockCivilTimes& b) {
    return same_optional(a.utc, b.utc) && same_optional(a.de, b.de);
}
bool same_input(const BigClockDigitalInput& a, const BigClockDigitalInput& b) {
    return same_times(a.actual, b.actual) && a.preview.has_value() == b.preview.has_value() &&
           (!a.preview || same_times(*a.preview, *b.preview)) && a.synchronization == b.synchronization;
}
bool same_preferences(const BigClockPreferences& a, const BigClockPreferences& b) {
    return a.face == b.face && a.zone == b.zone && a.hour_format == b.hour_format &&
           a.show_date == b.show_date && a.show_seconds == b.show_seconds &&
           a.show_alarms == b.show_alarms && a.show_countdown == b.show_countdown &&
           a.show_de_weather == b.show_de_weather && a.show_space_weather == b.show_space_weather;
}
BigClockPreferences preferences() {
    return {BigClockFace::Digital, BigClockZone::Utc, BigClockHourFormat::TwentyFour,
            true, true, true, true, true, true};
}
const BigClockCivilTime utc{1970, 1, 1, 0, 0, 0}; // Valid epoch-zero civil representation.
const BigClockCivilTime de{1970, 1, 1, 9, 15, 30}; // Supplied DE; no conversion inferred.
const BigClockCivilTime preview_utc{2000, 2, 29, 12, 34, 56};
const BigClockCivilTime preview_de{2000, 3, 1, 23, 59, 59};
} // namespace

int main() {
    using namespace hamclock;
    auto settings = preferences();
    BigClockDigitalInput input{{utc, de}, std::nullopt, SyncStatus::Unknown};
    auto epoch = present_big_clock_digital(settings, input);
    check(epoch.status == BigClockDigitalStatus::Ready && epoch.synchronization == SyncStatus::Unknown &&
          text_equal(epoch.time_text, "00:00:00") && text_equal(epoch.date_text, "1970-01-01"),
          "valid epoch zero is not missing time or synchronization evidence");

    // Independent snprintf oracle for all 86,400 clock-field instants, both
    // hour formats and seconds/date toggles. Production uses no printf/locale.
    for (int hour = 0; hour < 24; ++hour) {
        for (int minute = 0; minute < 60; ++minute) {
            for (int second = 0; second < 60; ++second) {
                input.actual.utc = BigClockCivilTime{2000, 2, 29, hour, minute, second};
                for (unsigned flags = 0; flags < 8; ++flags) {
                    settings.hour_format = (flags & 1U) ? BigClockHourFormat::Twelve : BigClockHourFormat::TwentyFour;
                    settings.show_seconds = (flags & 2U) != 0;
                    settings.show_date = (flags & 4U) != 0;
                    std::array<char, 12> expected{};
                    const bool twelve = (flags & 1U) != 0;
                    const int shown = twelve ? (hour == 0 || hour == 12 ? 12 : hour > 12 ? hour - 12 : hour) : hour;
                    if (settings.show_seconds) std::snprintf(expected.data(), expected.size(), "%02d:%02d:%02d", shown, minute, second);
                    else std::snprintf(expected.data(), expected.size(), "%02d:%02d", shown, minute);
                    if (twelve) {
                        const std::size_t offset = settings.show_seconds ? 8 : 5;
                        expected[offset] = ' '; expected[offset + 1] = hour < 12 ? 'A' : 'P';
                        expected[offset + 2] = 'M';
                    }
                    const auto allocations_before = allocation_count;
                    const auto result = present_big_clock_digital(settings, input);
                    check(allocation_count == allocations_before, "all valid time/format paths avoid C++ heap allocations");
                    check(result.time_text == expected, "exhaustive 12/24-hour second/toggle oracle and unused zero bytes");
                    check(text_equal(result.date_text, settings.show_date ? "2000-02-29" : ""),
                          "date desire independent of time format and owned zero bytes");
                }
            }
        }
    }

    // Independent month-end oracle uses an explicit 400-year cycle, including
    // leap centuries, and probes every day through 32 for all twelve months.
    constexpr int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (int year = 1801; year <= 2200; ++year) {
        const bool leap = year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
        for (int month = 1; month <= 12; ++month) {
            const int last = days[month - 1] + (month == 2 && leap ? 1 : 0);
            for (int day = 0; day <= 32; ++day) {
                const BigClockCivilTime civil{year, month, day, 23, 59, 59};
                check(rejects([&] { validate_big_clock_civil_time(civil); }) == (day == 0 || day > last),
                      "Gregorian complete month/day cycle with leap centuries");
            }
        }
    }
    settings = preferences();
    for (const auto civil : {BigClockCivilTime{1,1,1,0,0,0}, BigClockCivilTime{9999,12,31,23,59,59}}) {
        input = {{civil, std::nullopt}, std::nullopt, SyncStatus::Unsynchronized};
        const auto result = present_big_clock_digital(settings, input);
        check(result.status == BigClockDigitalStatus::Ready && result.synchronization == SyncStatus::Unsynchronized,
              "calendar endpoints valid despite unsynchronized evidence");
        check(text_equal(result.date_text, civil.year == 1 ? "0001-01-01" : "9999-12-31"),
              "exact four-digit endpoint date");
    }

    // All sixteen source-presence combinations, optional preview bundle, both
    // zones, all sync states and both faces: presence is not source accuracy.
    for (unsigned mask = 0; mask < 16; ++mask) {
        for (const bool preview : {false, true}) {
            for (const auto zone : {BigClockZone::Utc, BigClockZone::De}) {
                for (const auto sync : {SyncStatus::Unknown, SyncStatus::Synchronized, SyncStatus::Unsynchronized}) {
                    for (const auto face : {BigClockFace::Digital, BigClockFace::Analog}) {
                        settings = preferences(); settings.zone = zone; settings.face = face;
                        input = {{(mask & 1U) ? std::optional<BigClockCivilTime>{utc} : std::nullopt,
                                  (mask & 2U) ? std::optional<BigClockCivilTime>{de} : std::nullopt},
                                 std::nullopt, sync};
                        if (preview) input.preview = BigClockCivilTimes{
                            (mask & 4U) ? std::optional<BigClockCivilTime>{preview_utc} : std::nullopt,
                            (mask & 8U) ? std::optional<BigClockCivilTime>{preview_de} : std::nullopt};
                        const auto before = input;
                        const auto original_preferences = settings;
                        const bool actual_available = (mask & (zone == BigClockZone::Utc ? 1U : 2U)) != 0;
                        const bool selected_available = preview ? (mask & (zone == BigClockZone::Utc ? 4U : 8U)) != 0 : actual_available;
                        const auto view = present_big_clock_digital(settings, input);
                        check(view.status == (face == BigClockFace::Analog ? BigClockDigitalStatus::UnsupportedFace :
                              selected_available ? BigClockDigitalStatus::Ready : BigClockDigitalStatus::MissingSelectedTime),
                              "missing selected source and unsupported face never fall back");
                        check(view.selected_time_available == selected_available &&
                              view.actual_selected_time_available == actual_available && view.synchronization == sync &&
                              view.basis == (preview ? BigClockDisplayBasis::Preview : BigClockDisplayBasis::Actual),
                              "actual availability, preview and synchronization remain distinct");
                        check(text_equal(view.zone_text, zone == BigClockZone::Utc ? "UTC" : "DE"),
                              "zone identity preserved even unavailable");
                        if (face == BigClockFace::Analog || !selected_available) {
                            check(text_equal(view.time_text, "") && text_equal(view.date_text, ""),
                                  "unsupported or missing result has all-zero strings");
                        } else {
                            const char* time = preview ? (zone == BigClockZone::Utc ? "12:34:56" : "23:59:59") :
                                                       (zone == BigClockZone::Utc ? "00:00:00" : "09:15:30");
                            const char* date = preview ? (zone == BigClockZone::Utc ? "2000-02-29" : "2000-03-01") : "1970-01-01";
                            check(text_equal(view.time_text, time) && text_equal(view.date_text, date), "selected injected civil source exactly formatted");
                        }
                        check(same_input(input, before) && same_preferences(settings, original_preferences) &&
                              same_preferences(view.preferences, original_preferences), "all desires and inputs immutable");
                    }
                }
            }
        }
    }

    // Extremes in every civil field: bounds checked before indexing/arithmetic.
    const std::array<int BigClockCivilTime::*, 6> fields = {&BigClockCivilTime::year, &BigClockCivilTime::month,
        &BigClockCivilTime::day, &BigClockCivilTime::hour, &BigClockCivilTime::minute, &BigClockCivilTime::second};
    const std::array<int, 6> upper = {9999, 12, 31, 23, 59, 59};
    for (std::size_t field = 0; field < fields.size(); ++field) {
        for (const int value : {INT_MIN, -1, upper[field] + 1, INT_MAX}) {
            auto invalid = utc; invalid.*fields[field] = value;
            check(rejects([&] { validate_big_clock_civil_time(invalid); }), "civil extremes reject before arithmetic");
            for (unsigned location = 0; location < 4; ++location) {
                input = {{utc, de}, BigClockCivilTimes{preview_utc, preview_de}, SyncStatus::Unknown};
                if (location == 0) input.actual.utc = invalid;
                if (location == 1) input.actual.de = invalid;
                if (location == 2) input.preview->utc = invalid;
                if (location == 3) input.preview->de = invalid;
                settings = preferences(); settings.face = BigClockFace::Analog;
                const auto before = input;
                check(rejects([&] { present_big_clock_digital(settings, input); }),
                      "invalid hidden/unselected civil rejected even for unsupported analog");
                check(same_input(input, before), "invalid formatter leaves full input unchanged");
            }
        }
    }
    auto leap_second = utc; leap_second.second = 60;
    check(rejects([&] { validate_big_clock_civil_time(leap_second); }), "second 60 not silently normalized");
    for (unsigned field = 0; field < 3; ++field) {
        auto invalid = utc; invalid.*fields[field] = 0;
        check(rejects([&] { validate_big_clock_civil_time(invalid); }), "zero year/month/day invalid");
    }

    settings = preferences(); input = {{utc, de}, std::nullopt, SyncStatus::Unknown};
    // SyncStatus is scoped, with fixed implicit int underlying type. Any int
    // value is representable as this enum, but only its three named values are
    // valid for this product contract. See C++ draft [dcl.enum]/5,8 and
    // [expr.static.cast]/8; unnamed representable values are not cast UB.
    static_assert(std::is_same_v<std::underlying_type_t<SyncStatus>, int>);
    for (unsigned value = 0; value < 4; ++value) {
        input.synchronization = static_cast<SyncStatus>(value);
        check(rejects([&] { present_big_clock_digital(settings, input); }) == (value == 3),
              "known sync enums accepted, representable unnamed enum rejected");
    }
    // Unknown int extremes must reject on every source/selection path, even
    // missing/hidden civil data or an unsupported analog desire. Neither input
    // evidence nor the full preference value may be rewritten on rejection.
    for (const int value : {INT_MIN, -1, 255, INT_MAX}) {
        for (unsigned mask = 0; mask < 16; ++mask) {
            for (const bool preview : {false, true}) {
                for (const auto zone : {BigClockZone::Utc, BigClockZone::De}) {
                    for (const auto face : {BigClockFace::Digital, BigClockFace::Analog}) {
                        auto wanted = preferences(); wanted.zone = zone; wanted.face = face;
                        BigClockDigitalInput unknown{
                            {(mask & 1U) ? std::optional<BigClockCivilTime>{utc} : std::nullopt,
                             (mask & 2U) ? std::optional<BigClockCivilTime>{de} : std::nullopt},
                            std::nullopt, static_cast<SyncStatus>(value)};
                        if (preview) unknown.preview = BigClockCivilTimes{
                            (mask & 4U) ? std::optional<BigClockCivilTime>{preview_utc} : std::nullopt,
                            (mask & 8U) ? std::optional<BigClockCivilTime>{preview_de} : std::nullopt};
                        const auto before = unknown;
                        const auto before_preferences = wanted;
                        check(rejects([&] { present_big_clock_digital(wanted, unknown); }),
                              "representable unknown int sync values reject even on missing/hidden/analog paths");
                        check(same_input(unknown, before) && same_preferences(wanted, before_preferences),
                              "unknown sync rejection preserves all inputs and desires");
                    }
                }
            }
        }
    }
    input.synchronization = SyncStatus::Unknown;
    for (unsigned value = 0; value < 256; ++value) {
        for (unsigned field = 0; field < 3; ++field) {
            auto invalid = settings;
            if (field == 0) invalid.face = static_cast<BigClockFace>(value);
            if (field == 1) invalid.zone = static_cast<BigClockZone>(value);
            if (field == 2) invalid.hour_format = static_cast<BigClockHourFormat>(value);
            check(rejects([&] { present_big_clock_digital(invalid, input); }) == (value > 1),
                  "all preference enum spellings validated before formatting");
        }
    }

    for (unsigned bits = 0; bits < 512; ++bits) {
        const BigClockPreferences wanted{
            (bits & 1U) ? BigClockFace::Digital : BigClockFace::Analog,
            (bits & 2U) ? BigClockZone::De : BigClockZone::Utc,
            (bits & 4U) ? BigClockHourFormat::TwentyFour : BigClockHourFormat::Twelve,
            (bits & 8U) != 0, (bits & 16U) != 0, (bits & 32U) != 0,
            (bits & 64U) != 0, (bits & 128U) != 0, (bits & 256U) != 0};
        const auto view = present_big_clock_digital(wanted, input);
        check(same_preferences(view.preferences, wanted), "all nine desires preserved for every preference combination");
    }

    // Returned arrays own their data and success performs no heap allocation.
    input = {{utc, de}, std::nullopt, SyncStatus::Unknown}; settings = preferences();
    const auto retained = present_big_clock_digital(settings, input);
    input.actual.utc = preview_utc;
    const auto allocation_before = allocation_count;
    const auto later = present_big_clock_digital(settings, input);
    const bool no_heap = allocation_count == allocation_before;
    check(no_heap, "valid presentation makes no C++ heap allocations");
    check(text_equal(retained.time_text, "00:00:00") && text_equal(later.time_text, "12:34:56"),
          "owned previous output unchanged by next source or presentation");
    input.preview = BigClockCivilTimes{};
    const auto missing_before = allocation_count;
    const auto missing = present_big_clock_digital(settings, input);
    check(allocation_count == missing_before && missing.status == BigClockDigitalStatus::MissingSelectedTime,
          "missing preview success has no heap or actual fallback");
    std::cout << "Big Clock presentation: " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
