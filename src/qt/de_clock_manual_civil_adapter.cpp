#include "de_clock_manual_civil_adapter.hpp"
#include <limits>

namespace hamclock {
DeClockManualCivilResult adapt_de_clock_manual_civil(
    const DeClockConfiguration& configuration, const DeClockOffsetPolicy& policy,
    std::optional<UtcSeconds> actual) {
    validate_de_clock_configuration(configuration, policy);
    DeClockManualCivilResult result{DeClockManualCivilStatus::MissingSource, configuration.mode,
                                   actual, configuration.manual_offset, std::nullopt, std::nullopt};
    if (!actual) return result;
    if (configuration.mode == DeClockMode::Automatic) {
        result.status = DeClockManualCivilStatus::AutomaticUnresolved;
        return result;
    }
    const auto source = actual->value;
    if (source < kBigClockUtcCivilMinimumSeconds || source > kBigClockUtcCivilMaximumSeconds) {
        result.status = DeClockManualCivilStatus::SourceOutOfRange;
        return result;
    }
    const auto offset = configuration.manual_offset->value;
    // Subtract only within representable endpoint ranges. In particular,
    // minimum - offset is valid for negative offset, even offset==INT64_MIN;
    // never negate the offset or add before validating the mathematical sum.
    if ((offset > 0 && source > std::numeric_limits<std::int64_t>::max() - offset) ||
        (offset < 0 && source < std::numeric_limits<std::int64_t>::min() - offset)) {
        result.status = DeClockManualCivilStatus::OffsetOverflow;
        return result;
    }
    const auto shifted = source + offset;
    result.shifted_calendar = DeClockShiftedCivilSeconds{shifted};
    if (shifted < kBigClockUtcCivilMinimumSeconds || shifted > kBigClockUtcCivilMaximumSeconds) {
        result.status = DeClockManualCivilStatus::DisplayOutOfRange;
        return result;
    }
    // The UTC adapter is used solely as a guarded calendar decoder. Its input
    // is this local Unix-shaped encoding; never expose it as actual UTC source.
    const auto converted = adapt_big_clock_utc_civil(UtcSeconds{shifted});
    if (converted.status != BigClockUtcCivilStatus::Ready || !converted.civil ||
        !converted.source || converted.source->value != shifted) {
        result.status = DeClockManualCivilStatus::InvalidConversion;
        return result;
    }
    result.status = DeClockManualCivilStatus::Ready;
    result.civil = converted.civil;
    return result;
}
} // namespace hamclock
