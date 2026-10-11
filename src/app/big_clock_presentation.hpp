#pragma once

#include "big_clock_preferences.hpp"
#include "../domain/clock_state.hpp"
#include <array>
#include <cstddef>
#include <optional>

namespace hamclock {

// Original bounded civil-field display contract; no UTC-to-civil conversion,
// system clock, host/local zone inference, DST, leap-second support or source
// accuracy guarantee. Gregorian year 1..9999; valid month/day including leap
// years divisible by 4 except centuries not divisible by 400. Hour 0..23,
// minute/second 0..59. No normalization of impossible dates or second 60.
// These format/range policies are replacement choices, not original limits.
struct BigClockCivilTime {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};
void validate_big_clock_civil_time(const BigClockCivilTime&);

// Each optional is an explicitly supplied value for its named zone. Absence
// means missing source/conversion for THAT zone, never epoch zero or Unknown
// synchronization. Caller supplies reviewed conversion/provenance separately;
// this API cannot verify that UTC/DE values describe the same instant.
struct BigClockCivilTimes {
    std::optional<BigClockCivilTime> utc;
    std::optional<BigClockCivilTime> de;
};
struct BigClockDigitalInput {
    BigClockCivilTimes actual;
    // Presence explicitly selects preview, even if offset was zero or both
    // preview-zone values are unavailable. Never fall back to actual time when
    // the selected preview value is missing. No preview arithmetic occurs here.
    std::optional<BigClockCivilTimes> preview;
    // Evidence about actual clock synchronization, independent of civil-value
    // availability and preview. Unknown/Unsynchronized do not erase valid time;
    // Synchronized does not fabricate missing time or certify its provenance.
    SyncStatus synchronization;
};

enum class BigClockDisplayBasis { Actual, Preview };
enum class BigClockDigitalStatus { Ready, MissingSelectedTime, UnsupportedFace };
constexpr std::size_t kBigClockTimeTextBytes = 12; // HH:MM:SS AM + NUL
constexpr std::size_t kBigClockDateTextBytes = 11; // YYYY-MM-DD + NUL
struct BigClockDigitalView {
    BigClockPreferences preferences;
    BigClockDisplayBasis basis;
    SyncStatus synchronization;
    BigClockDigitalStatus status;
    // Availability means a valid injected civil value exists, not freshness,
    // synchronization or live/source-quality evidence. Preserve actual selected
    // zone availability separately while rendering a preview.
    bool selected_time_available;
    bool actual_selected_time_available;
    std::array<char, kBigClockTimeTextBytes> time_text;
    std::array<char, kBigClockDateTextBytes> date_text;
    std::array<char, 4> zone_text; // Always UTC or DE with NUL, even unavailable.
};

// Pure bounded owned output. Validate all preference enums, synchronization
// enum and EVERY present civil value (including unselected zones and actual
// values hidden by preview) before return. Invalid input throws invalid_argument;
// inputs are never mutated and there is no partially committed shared state.
// Successful formatting allocates no heap storage; exception construction may.
//
// Copy all nine preferences without rewrite; no weather/alarm/countdown/analog
// functionality or capability claims. Analog desire => UnsupportedFace, empty
// time/date, while availability/basis/sync/zone remain truthful. It must not be
// silently rendered as a digital replacement. Otherwise absent selected-zone
// value => MissingSelectedTime and empty time/date; no other-zone/time fallback.
// Digital + present selected civil value => Ready regardless of sync evidence.
//
// 24-hour time is HH:MM or HH:MM:SS; 12-hour is hh:MM AM/PM or hh:MM:SS AM/PM,
// midnight 12 AM, noon 12 PM, using ASCII and two-digit hours. show_seconds alone
// controls the seconds field. show_date produces YYYY-MM-DD, otherwise empty.
// No locale/month/day-of-week conversion. All arrays NUL-terminated and unused
// bytes zero; absent strings are all-zero arrays. The UTC/DE label is independent
// of formatted value. A later UI supplies explicit Preview/sync/error status
// text and must not portray Preview or Unknown sync as synchronized actual time.
BigClockDigitalView present_big_clock_digital(
    const BigClockPreferences&, const BigClockDigitalInput&);

} // namespace hamclock
