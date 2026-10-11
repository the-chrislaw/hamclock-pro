#include "big_clock_preferences.hpp"

#include <stdexcept>

namespace hamclock {
namespace {
bool equal(const BigClockPreferences& a, const BigClockPreferences& b) noexcept {
    return a.face == b.face && a.zone == b.zone && a.hour_format == b.hour_format &&
           a.show_date == b.show_date && a.show_seconds == b.show_seconds &&
           a.show_alarms == b.show_alarms && a.show_countdown == b.show_countdown &&
           a.show_de_weather == b.show_de_weather &&
           a.show_space_weather == b.show_space_weather;
}
void validate_capability(BigClockCapability capability) {
    if (capability != BigClockCapability::Unavailable &&
        capability != BigClockCapability::Available)
        throw std::invalid_argument("Unknown Big Clock capability");
}
} // namespace

void validate_big_clock_preferences(const BigClockPreferences& preferences) {
    if (preferences.face != BigClockFace::Analog && preferences.face != BigClockFace::Digital)
        throw std::invalid_argument("Unknown Big Clock face");
    if (preferences.zone != BigClockZone::Utc && preferences.zone != BigClockZone::De)
        throw std::invalid_argument("Unknown Big Clock zone");
    if (preferences.hour_format != BigClockHourFormat::Twelve &&
        preferences.hour_format != BigClockHourFormat::TwentyFour)
        throw std::invalid_argument("Unknown Big Clock hour format");
}

BigClockPreferenceState::BigClockPreferenceState(const BigClockPreferences& preferences)
    : preferences_(preferences) {
    validate_big_clock_preferences(preferences);
}

const BigClockPreferences& BigClockPreferenceState::preferences() const noexcept {
    return preferences_;
}

BigClockPreferenceResult BigClockPreferenceState::replace(const BigClockPreferences& candidate) {
    validate_big_clock_preferences(candidate);
    if (equal(preferences_, candidate)) return BigClockPreferenceResult::Unchanged;
    preferences_ = candidate;
    return BigClockPreferenceResult::Applied;
}

void validate_big_clock_capabilities(const BigClockCapabilities& capabilities) {
    validate_capability(capabilities.analog_face);
    validate_capability(capabilities.de_civil_time);
    validate_capability(capabilities.alarms);
    validate_capability(capabilities.countdown);
    validate_capability(capabilities.de_weather);
    validate_capability(capabilities.space_weather);
}

BigClockPreferenceEvaluation evaluate_big_clock_preferences(
    const BigClockPreferences& preferences, const BigClockCapabilities& capabilities) {
    validate_big_clock_preferences(preferences);
    validate_big_clock_capabilities(capabilities);
    return {{preferences.face == BigClockFace::Analog, capabilities.analog_face},
            {preferences.zone == BigClockZone::De, capabilities.de_civil_time},
            {preferences.show_alarms, capabilities.alarms},
            {preferences.show_countdown, capabilities.countdown},
            {preferences.show_de_weather, capabilities.de_weather},
            {preferences.show_space_weather, capabilities.space_weather}};
}
} // namespace hamclock
