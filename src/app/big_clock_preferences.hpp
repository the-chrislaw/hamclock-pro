#pragma once

#include <cstdint>

namespace hamclock {

// Original replacement API proposal, not original HamClock enum values,
// defaults or a parity certificate. Header contract awaits independent gate.
enum class BigClockFace : std::uint8_t { Analog = 0, Digital = 1 };
enum class BigClockZone : std::uint8_t { Utc = 0, De = 1 };
enum class BigClockHourFormat : std::uint8_t { Twelve = 0, TwentyFour = 1 };

// Explicit caller configuration: no implicit original/product defaults.
// Weather desires are independent so this API can represent neither, either
// or BOTH. The guide does not establish the original simultaneous-weather
// policy; no mutual-exclusion limitation is introduced here.
// These are display desires only. They never arm an alarm, start/restart a
// countdown, choose clock preview, obtain weather or convert DE civil time.
// Seconds is an abstract display desire; analog hand/digital format policy
// remains separate. Twelve/TwentyFour on Analog is retained, not normalized.
struct BigClockPreferences {
    BigClockFace face;
    BigClockZone zone;
    BigClockHourFormat hour_format;
    bool show_date;
    bool show_seconds;
    bool show_alarms;
    bool show_countdown;
    bool show_de_weather;
    bool show_space_weather;
};

// Throws invalid_argument for any unrecognized face/zone/hour enum. Every
// combination of recognized enums/booleans is valid, including both weather
// desires and unavailable features. Availability never rewrites preferences.
void validate_big_clock_preferences(const BigClockPreferences&);

enum class BigClockPreferenceResult : std::uint8_t { Applied, Unchanged };

// Bounded in-memory preference state only. Requires explicit validated config;
// no default constructor, IO, Qt, time conversion, persistence, provider data,
// network, audio, hardware or scheduler. Copyable value state, no heap members.
// replace validates the full candidate before commit, including a candidate
// equal to current settings. Any exception leaves the entire state unchanged.
// Successful replace allocates no storage and returns Unchanged only when all
// nine fields compare equal. No source validity or rendering promise is made.
class BigClockPreferenceState {
public:
    explicit BigClockPreferenceState(const BigClockPreferences&);
    const BigClockPreferences& preferences() const noexcept;
    BigClockPreferenceResult replace(const BigClockPreferences&);
private:
    BigClockPreferences preferences_;
};

// Capability means only that the caller supplies a reviewed implementation
// for that feature. Available is NOT Current/Live, a valid reading, an armed
// alarm, a running countdown, accurate time, licensed data or AX compliance.
// Actual source quality/freshness/time validity must travel in a separate view
// snapshot. Missing source time cannot be inferred from this preference API.
enum class BigClockCapability : std::uint8_t { Unavailable = 0, Available = 1 };
struct BigClockCapabilities {
    BigClockCapability analog_face;
    BigClockCapability de_civil_time;
    BigClockCapability alarms;
    BigClockCapability countdown;
    BigClockCapability de_weather;
    BigClockCapability space_weather;
};

// Validate EVERY capability enum, including unrequested features; throws
// invalid_argument on unknown values. Does not certify caller claims.
void validate_big_clock_capabilities(const BigClockCapabilities&);

struct BigClockRequirement {
    bool requested;
    BigClockCapability capability;
};
struct BigClockPreferenceEvaluation {
    BigClockRequirement analog_face;
    BigClockRequirement de_civil_time;
    BigClockRequirement alarms;
    BigClockRequirement countdown;
    BigClockRequirement de_weather;
    BigClockRequirement space_weather;
};

// Pure validation + classification, no mutations or fallback selection. Returns
// requested=true for Analog face, De zone and each corresponding desire; copies
// caller capability independently, even if not requested. Requested+Unavailable
// must be represented truthfully by a later UI, not silently changed to UTC,
// Digital, another weather product or a fabricated zero/current observation.
// Validates both complete inputs before returning; no Qt/IO/clock/provider work.
// Date/seconds/hour format remain preferences, not additional capability claims.
BigClockPreferenceEvaluation evaluate_big_clock_preferences(
    const BigClockPreferences&, const BigClockCapabilities&);

} // namespace hamclock
