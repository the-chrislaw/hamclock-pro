#include "../src/app/big_clock_preferences.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {
using namespace hamclock;
int checks = 0;
int failures = 0;
void check(bool condition, const char* label) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << label << '\n';
    }
}
template<class Action> bool rejects(Action action) {
    try { action(); }
    catch (const std::invalid_argument&) { return true; }
    return false;
}
bool equal(const BigClockPreferences& a, const BigClockPreferences& b) {
    return a.face == b.face && a.zone == b.zone && a.hour_format == b.hour_format &&
           a.show_date == b.show_date && a.show_seconds == b.show_seconds &&
           a.show_alarms == b.show_alarms && a.show_countdown == b.show_countdown &&
           a.show_de_weather == b.show_de_weather && a.show_space_weather == b.show_space_weather;
}
BigClockPreferences configuration(unsigned bits) {
    return {(bits & 1U) ? BigClockFace::Digital : BigClockFace::Analog,
            (bits & 2U) ? BigClockZone::De : BigClockZone::Utc,
            (bits & 4U) ? BigClockHourFormat::TwentyFour : BigClockHourFormat::Twelve,
            (bits & 8U) != 0, (bits & 16U) != 0, (bits & 32U) != 0,
            (bits & 64U) != 0, (bits & 128U) != 0, (bits & 256U) != 0};
}
BigClockCapabilities capabilities(unsigned bits) {
    const auto value = [bits](unsigned bit) {
        return (bits & bit) ? BigClockCapability::Available : BigClockCapability::Unavailable;
    };
    return {value(1), value(2), value(4), value(8), value(16), value(32)};
}
bool requirement(const BigClockRequirement& result, bool requested, bool available) {
    return result.requested == requested && result.capability ==
           (available ? BigClockCapability::Available : BigClockCapability::Unavailable);
}
} // namespace

int main() {
    static_assert(!std::is_default_constructible_v<BigClockPreferenceState>);
    static_assert(std::is_trivially_copyable_v<BigClockPreferences>);
    static_assert(std::is_nothrow_copy_assignable_v<BigClockPreferences>);
    static_assert(std::is_trivially_copyable_v<BigClockPreferenceEvaluation>);
    using namespace hamclock;

    // Independent exhaustive truth table: 2^9 preference combinations and
    // 2^6 capability combinations, including both weather desires together.
    BigClockPreferenceState state(configuration(0));
    for (unsigned bits = 0; bits < 512; ++bits) {
        const auto wanted = configuration(bits);
        validate_big_clock_preferences(wanted);
        const auto prior = state.preferences();
        check(state.replace(wanted) == (equal(prior, wanted) ? BigClockPreferenceResult::Unchanged :
              BigClockPreferenceResult::Applied), "replace result classifies whole preference change");
        check(equal(state.preferences(), wanted), "all desired fields retained exactly");
        check(state.replace(wanted) == BigClockPreferenceResult::Unchanged,
              "repeat valid configuration is unchanged");
        BigClockPreferenceState copied(state);
        check(equal(copied.preferences(), wanted), "copy retains explicit complete configuration");
        for (unsigned cap = 0; cap < 64; ++cap) {
            const auto supplied = capabilities(cap);
            const auto view = evaluate_big_clock_preferences(wanted, supplied);
            check(requirement(view.analog_face, (bits & 1U) == 0, (cap & 1U) != 0),
                  "analog request independent of implementation availability");
            check(requirement(view.de_civil_time, (bits & 2U) != 0, (cap & 2U) != 0),
                  "DE desire never silently rewritten to UTC");
            check(requirement(view.alarms, (bits & 32U) != 0, (cap & 4U) != 0),
                  "alarm display desire is not arming or live-data evidence");
            check(requirement(view.countdown, (bits & 64U) != 0, (cap & 8U) != 0),
                  "countdown desire not changed by capability");
            check(requirement(view.de_weather, (bits & 128U) != 0, (cap & 16U) != 0),
                  "DE weather desire retained independently");
            check(requirement(view.space_weather, (bits & 256U) != 0, (cap & 32U) != 0),
                  "space weather desire retained independently");
            check(equal(state.preferences(), wanted), "pure evaluation never rewrites state");
        }
        // Single-field transitions verify comparison includes every preference.
        for (unsigned field = 0; field < 9; ++field) {
            BigClockPreferenceState single(wanted);
            const auto changed = configuration(bits ^ (1U << field));
            check(single.replace(changed) == BigClockPreferenceResult::Applied,
                  "every individual field participates in change detection");
            check(equal(single.preferences(), changed), "single-field transition commits complete value");
            check(equal(copied.preferences(), wanted), "copied state independent of changed state");
        }
    }

    // Every possible spelling of each fixed uint8 enum, not only a sentinel.
    for (unsigned field = 0; field < 3; ++field) {
        for (unsigned value = 0; value < 256; ++value) {
            auto candidate = configuration(511);
            if (field == 0) candidate.face = static_cast<BigClockFace>(value);
            if (field == 1) candidate.zone = static_cast<BigClockZone>(value);
            if (field == 2) candidate.hour_format = static_cast<BigClockHourFormat>(value);
            const bool bad = value > 1;
            check(rejects([&] { validate_big_clock_preferences(candidate); }) == bad,
                  "all preference enum spellings validated");
            check(rejects([&] { BigClockPreferenceState constructed(candidate); (void)constructed; }) == bad,
                  "constructor rejects every unknown enum");
            BigClockPreferenceState guarded(configuration(170));
            const auto before = guarded.preferences();
            check(rejects([&] { guarded.replace(candidate); }) == bad,
                  "replace rejects every unknown preference enum");
            check(equal(guarded.preferences(), bad ? before : candidate),
                  "rejected replacement preserves every prior field");
            check(rejects([&] { evaluate_big_clock_preferences(candidate, capabilities(63)); }) == bad,
                  "evaluation validates preference enums despite available capabilities");
            if (bad) {
                check(rejects([&] { guarded.replace(candidate); }), "repeat invalid candidate still rejected");
                check(equal(guarded.preferences(), before), "repeat invalid candidate preserves state");
            }
        }
    }

    const std::array<BigClockCapability BigClockCapabilities::*, 6> members = {
        &BigClockCapabilities::analog_face, &BigClockCapabilities::de_civil_time,
        &BigClockCapabilities::alarms, &BigClockCapabilities::countdown,
        &BigClockCapabilities::de_weather, &BigClockCapabilities::space_weather};
    // Digital/UTC with all optional features off: each capability is unrequested
    // and must STILL be checked. Also repeat with all corresponding desires on.
    for (const auto member : members) {
        for (unsigned value = 0; value < 256; ++value) {
            auto supplied = capabilities(0);
            supplied.*member = static_cast<BigClockCapability>(value);
            const bool bad = value > 1;
            check(rejects([&] { validate_big_clock_capabilities(supplied); }) == bad,
                  "all capability enum spellings checked");
            for (const unsigned bits : {1U, 510U}) {
                const auto wanted = configuration(bits);
                BigClockPreferenceState guarded(wanted);
                check(rejects([&] { evaluate_big_clock_preferences(wanted, supplied); }) == bad,
                      "unknown capability rejected even when feature unrequested");
                check(equal(guarded.preferences(), wanted), "capability rejection cannot rewrite desire");
            }
        }
    }

    // Explicit named acceptance fixture: Analog+24h, DE and BOTH weather
    // desires survive a completely unavailable capability snapshot unchanged.
    const auto dual_weather = configuration(510);
    const auto unavailable = evaluate_big_clock_preferences(dual_weather, capabilities(0));
    check(unavailable.analog_face.requested && unavailable.de_civil_time.requested &&
          unavailable.de_weather.requested && unavailable.space_weather.requested &&
          unavailable.analog_face.capability == BigClockCapability::Unavailable &&
          dual_weather.hour_format == BigClockHourFormat::TwentyFour,
          "no undocumented weather exclusivity, face normalization or fallback");
    std::cout << "Big Clock preferences: " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
