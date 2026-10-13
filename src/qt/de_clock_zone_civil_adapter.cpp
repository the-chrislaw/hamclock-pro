#include "de_clock_zone_civil_adapter.hpp"

namespace hamclock {
DeClockZoneCivilResult adapt_de_clock_zone_civil(const std::string& iana_id, std::optional<UtcSeconds> actual) {
    DeClockZoneCivilResult result{DeClockZoneCivilStatus::MissingSource, std::nullopt, std::nullopt};
    if (!actual) return result;
    if (actual->value < kBigClockUtcCivilMinimumSeconds || actual->value > kBigClockUtcCivilMaximumSeconds) {
        result.status = DeClockZoneCivilStatus::SourceOutOfRange;
        return result;
    }
    result.offset = resolve_de_zone_offset(iana_id, *actual);
    if (!result.offset) {
        result.status = DeClockZoneCivilStatus::UnknownZone;
        return result;
    }
    // Offsets are at most a day, so the sum cannot overflow; the UTC adapter is
    // used only as a guarded calendar decoder for the shifted encoding.
    const auto shifted = actual->value + result.offset->value;
    const auto converted = adapt_big_clock_utc_civil(UtcSeconds{shifted});
    if (converted.status != BigClockUtcCivilStatus::Ready || !converted.civil) {
        result.status = DeClockZoneCivilStatus::ConversionFailed;
        return result;
    }
    result.status = DeClockZoneCivilStatus::Ready;
    result.civil = converted.civil;
    return result;
}
} // namespace hamclock
