#include "de_clock_display_snapshot.hpp"
#include <stdexcept>

namespace hamclock {
DeClockDisplaySnapshot make_de_clock_display_snapshot(
    const DeClockConfigurationSnapshot& configuration, std::optional<UtcSeconds> actual_source,
    SyncStatus synchronization) {
    // All structural checks precede adapter calls, including unavailable paths.
    validate_de_clock_configuration(configuration.configuration, configuration.offset_policy);
    if (!valid_location(configuration.location) || configuration.location.longitude == 180 ||
        configuration.configuration_revision == 0 || configuration.location_revision == 0 ||
        configuration.configuration_revision < configuration.location_revision) {
        throw std::invalid_argument("Invalid DE display configuration snapshot metadata");
    }
    switch (synchronization) {
    case SyncStatus::Unknown:
    case SyncStatus::Synchronized:
    case SyncStatus::Unsynchronized:
        break;
    default:
        throw std::invalid_argument("Unknown DE display synchronization evidence");
    }
    const auto utc = adapt_big_clock_utc_civil(actual_source);
    const auto de = adapt_de_clock_manual_civil(configuration.configuration, configuration.offset_policy, actual_source);
    const BigClockDigitalInput digital{
        {utc.status == BigClockUtcCivilStatus::Ready ? utc.civil : std::nullopt,
         de.status == DeClockManualCivilStatus::Ready ? de.civil : std::nullopt},
        std::nullopt, synchronization};
    return {configuration, actual_source, utc, de, digital};
}
} // namespace hamclock
