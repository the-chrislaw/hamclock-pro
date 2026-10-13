#include "de_clock_configuration.hpp"
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace hamclock {
namespace {
static_assert(std::is_nothrow_copy_constructible_v<DeClockConfigurationSnapshot>);
static_assert(std::is_nothrow_copy_assignable_v<DeClockConfigurationSnapshot>);
static_assert(std::is_nothrow_move_constructible_v<DeClockConfigurationTransition>);
Location canonical_location(Location location) {
    if (!valid_location(location)) throw std::invalid_argument("Invalid DE clock station location");
    if (location.longitude == 180) location.longitude = -180;
    return location;
}
bool same_configuration(const DeClockConfiguration& a, const DeClockConfiguration& b) noexcept {
    return a.mode == b.mode && a.zone_id == b.zone_id && a.manual_offset.has_value() == b.manual_offset.has_value() &&
           (!a.manual_offset || a.manual_offset->value == b.manual_offset->value);
}
DeClockConfigurationSnapshot initial_snapshot(const DeClockConfiguration& configuration,
                                              const DeClockOffsetPolicy& policy, Location location) {
    validate_de_clock_configuration(configuration, policy);
    return {configuration, policy, canonical_location(location), 1, 1};
}
bool valid_zone_char(char c) noexcept {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
           c == '_' || c == '+' || c == '-' || c == '/';
}
void validate_zone_id(const std::array<char, kDeClockZoneIdBytes>& id) {
    std::size_t length = 0;
    while (length < id.size() && id[length] != '\0') {
        if (!valid_zone_char(id[length])) throw std::invalid_argument("DE zone id has invalid characters");
        ++length;
    }
    if (length == id.size()) throw std::invalid_argument("DE zone id is not NUL-terminated");
    for (std::size_t i = length; i < id.size(); ++i) {
        if (id[i] != '\0') throw std::invalid_argument("DE zone id has non-canonical padding");
    }
}
} // namespace

std::string_view de_clock_zone_id(const DeClockConfiguration& configuration) noexcept {
    const auto& id = configuration.zone_id;
    std::size_t length = 0;
    while (length < id.size() && id[length] != '\0') ++length;
    return {id.data(), length};
}

DeClockConfiguration make_de_clock_zone_configuration(std::string_view iana_id) {
    if (iana_id.empty() || iana_id.size() >= kDeClockZoneIdBytes) {
        throw std::invalid_argument("DE zone id empty or too long");
    }
    DeClockConfiguration configuration{DeClockMode::Automatic, std::nullopt, {}};
    for (std::size_t i = 0; i < iana_id.size(); ++i) configuration.zone_id[i] = iana_id[i];
    validate_zone_id(configuration.zone_id);
    return configuration;
}

DeClockConfiguration de_clock_configuration_for_saved_zone(std::string_view saved_zone) {
    if (saved_zone.empty()) return DeClockConfiguration{DeClockMode::Automatic, std::nullopt, {}};
    return make_de_clock_zone_configuration(saved_zone);
}

void validate_de_clock_configuration(const DeClockConfiguration& configuration,
                                     const DeClockOffsetPolicy& policy) {
    if (policy.minimum.value > policy.maximum.value) {
        throw std::invalid_argument("DE manual offset policy bounds are reversed");
    }
    validate_zone_id(configuration.zone_id);
    switch (configuration.mode) {
    case DeClockMode::Automatic:
        if (configuration.manual_offset) throw std::invalid_argument("Automatic DE desire must not retain a manual offset");
        break;
    case DeClockMode::ManualOffset:
        if (configuration.zone_id[0] != '\0') throw std::invalid_argument("Manual DE desire must not retain a zone id");
        if (!configuration.manual_offset || configuration.manual_offset->value < policy.minimum.value ||
            configuration.manual_offset->value > policy.maximum.value) {
            throw std::invalid_argument("Manual DE offset missing or outside explicit caller bounds");
        }
        break;
    default:
        throw std::invalid_argument("Unknown DE clock mode");
    }
}
std::uint64_t next_de_clock_revision(std::uint64_t previous) {
    if (previous == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("DE clock revision namespace exhausted");
    }
    return previous + 1;
}

DeClockConfigurationState::DeClockConfigurationState(const DeClockConfiguration& configuration,
                                                    const DeClockOffsetPolicy& policy, Location location)
    : snapshot_(initial_snapshot(configuration, policy, location)) {}
DeClockConfigurationSnapshot DeClockConfigurationState::snapshot() const noexcept { return snapshot_; }
DeClockConfigurationTransition DeClockConfigurationState::replace(const DeClockConfiguration& configuration) {
    validate_de_clock_configuration(configuration, snapshot_.offset_policy);
    if (same_configuration(snapshot_.configuration, configuration)) {
        return {DeClockConfigurationResult::Unchanged, snapshot_};
    }
    auto candidate = snapshot_;
    candidate.configuration = configuration;
    candidate.configuration_revision = next_de_clock_revision(snapshot_.configuration_revision);
    const DeClockConfigurationTransition output{DeClockConfigurationResult::Applied, candidate};
    snapshot_ = candidate;
    return output;
}
DeClockConfigurationTransition DeClockConfigurationState::set_location(Location location) {
    location = canonical_location(location);
    if (location.latitude == snapshot_.location.latitude && location.longitude == snapshot_.location.longitude) {
        return {DeClockConfigurationResult::Unchanged, snapshot_};
    }
    auto candidate = snapshot_;
    candidate.location = location;
    candidate.configuration = {DeClockMode::Automatic, std::nullopt};
    candidate.configuration_revision = next_de_clock_revision(snapshot_.configuration_revision);
    candidate.location_revision = next_de_clock_revision(snapshot_.location_revision);
    const DeClockConfigurationTransition output{DeClockConfigurationResult::Applied, candidate};
    snapshot_ = candidate;
    return output;
}
} // namespace hamclock
