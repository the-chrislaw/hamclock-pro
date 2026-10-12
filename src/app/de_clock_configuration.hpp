#pragma once

#include "../domain/clock_state.hpp"
#include "../domain/geometry.hpp"
#include <optional>

namespace hamclock {
enum class DeClockMode { Automatic, ManualOffset };
struct DeClockOffsetPolicy {
    // Mandatory caller policy in signed seconds east of UTC, inclusive ordered
    // bounds. No original-guide numeric limit or implicit product cap inferred.
    UtcOffsetSeconds minimum;
    UtcOffsetSeconds maximum;
};
struct DeClockConfiguration {
    // Desire only: Automatic never means resolved/available/correct DE time.
    DeClockMode mode;
    // Present exactly for ManualOffset; Automatic with retained offset rejects.
    // Integer seconds, no rounding/step inference or automatic DST adjustment.
    std::optional<UtcOffsetSeconds> manual_offset;
};

// Validate policy even for Automatic: minimum<=maximum. Validate known mode,
// exact mode/payload relationship and manual offset within explicit bounds.
// Throw invalid_argument on any violation, no mutation. Numeric policy may
// exceed a later adapter's representable display range; this is configuration
// desire, not an offset arithmetic/civil conversion availability guarantee.
void validate_de_clock_configuration(const DeClockConfiguration&,
                                     const DeClockOffsetPolicy&);

enum class DeClockConfigurationResult { Applied, Unchanged };
struct DeClockConfigurationSnapshot {
    DeClockConfiguration configuration;
    DeClockOffsetPolicy offset_policy;
    Location location;
    // Instance-scoped counters, both start1, no persistence/global token claim.
    // config revision changes for different desire or changed location; location
    // revision changes only for changed location. Future resolution must bind
    // BOTH plus owning session lifetime/mode; no resolution authority issued.
    std::uint64_t configuration_revision;
    std::uint64_t location_revision;
};
struct DeClockConfigurationTransition {
    DeClockConfigurationResult result;
    DeClockConfigurationSnapshot snapshot;
};
// Pure full-range checked increment used before each revision commit. Maximum
// throws overflow_error rather than wrap/reuse; no test restoration backdoor.
std::uint64_t next_de_clock_revision(std::uint64_t previous);

// Original pure fixed-size session settings state. Mandatory initial desired
// configuration, explicit immutable bounds and validated station coordinates;
// no defaults, IANA mode, zone mapper/backend, source time/quality, host-local
// inference, Qt, IO/persistence, clock modification, alarms or DST scheduling.
// snapshot is an owned allocation-free value, no borrowed lifetime concerns.
// replace validates whole candidate before equality check; different desire
// increments config revision, same validated desire Unchanged/no counter change.
// set_location validates whole coordinates first. Exact latitude and longitude
// compare after canonicalizing +180 longitude to -180 (signed zero equivalent),
// no epsilon/nudge. Same location preserves manual desire/revisions. Changed
// location resets desired mode to Automatic/no manual payload and increments
// both counters, retaining the explicit offset policy. This tracks documented
// location-change reset, not a claim of complete original coordinate semantics.
// Invalid input or exhausted revision rejects atomically, preserving ALL fields;
// validate/prepare all revisions before commit. Successful operations allocate
// no heap storage; constructing standard exceptions may allocate. Session-only
// state is not documented restart persistence; integration/migration separate.
// Stable session owner: copy/move construction and assignment deleted so bounds
// and revision namespace cannot be replaced/rolled backward in the same object.
// Owned snapshots remain freely copyable values; they carry no token authority.
class DeClockConfigurationState {
public:
    DeClockConfigurationState(const DeClockConfiguration&, const DeClockOffsetPolicy&, Location);
    DeClockConfigurationState(const DeClockConfigurationState&) = delete;
    DeClockConfigurationState& operator=(const DeClockConfigurationState&) = delete;
    DeClockConfigurationState(DeClockConfigurationState&&) = delete;
    DeClockConfigurationState& operator=(DeClockConfigurationState&&) = delete;
    DeClockConfigurationSnapshot snapshot() const noexcept;
    DeClockConfigurationTransition replace(const DeClockConfiguration&);
    DeClockConfigurationTransition set_location(Location);
private:
    DeClockConfigurationSnapshot snapshot_;
};
} // namespace hamclock
