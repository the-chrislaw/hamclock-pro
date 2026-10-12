#pragma once

#include "big_clock_civil_adapter.hpp"
#include "de_clock_manual_civil_adapter.hpp"
#include "../app/de_clock_configuration.hpp"

namespace hamclock {
// One coherent owned synchronous publication value, not a second service,
// completed-request guard, provider cache or authoritative session token.
struct DeClockDisplaySnapshot {
    DeClockConfigurationSnapshot configuration;
    // Raw ACTUAL source retained exactly, including absence/epochzero/extremes.
    // Never replaced by a manual shifted-calendar encoding or preview clock.
    std::optional<UtcSeconds> actual_source;
    BigClockUtcCivilResult utc;
    // Includes independent manual offset/shifted-calendar/rawactual provenance
    // and explicit unresolved/missing/range/overflow/conversion failure reason.
    DeClockManualCivilResult de;
    // Civil fields and status provenance prepared from the SAME input source
    // and settings snapshot. actual.utc iff utc Ready; actual.de iff de Ready.
    // preview always absent; sync copied without source-validity inference.
    BigClockDigitalInput digital;
};

// Stateless conversion bridge for one serialized dashboard callback. Validate
// COMPLETE snapshot config/policy, finite/range-valid location, stored canonical
// longitude[-180,180), positive revisions and config_revision>=location_revision
// (reachable-state invariant), plus recognized SyncStatus before any conversion
// or early unavailable return. Signed zero coordinates remain valid; no nudge,
// guessed location or inferred host zone. Invalid schema throws invalid_argument.
// Metadata validates shape, not owning-session authenticity/history/currentness.
//
// Capture/copy one already validated caller-owned configuration snapshot and
// the optional cached ACTUAL source; never read OS time or call a getter inside
// conversion. Invoke frozen UTC and manual adapters on exactly that source.
// Their guard ranges/status precedence/provenance contracts are retained:
// missing source distinct from epochzero, Automatic unresolved without manual/
// host-local/UTC fallback, checked offset and both calendar bounds before Qt.
// Preserve original desired settings/policy/revisions and all raw provenance.
// Populate actual UTC/DE civil optionals only for respective Ready results;
// missing/error/out-of-range never becomes zero or another zone. Unknown or
// Unsynchronized evidence stays unchanged even if civil fields are available;
// Synchronized never invents missing time. No preview selection/arithmetic.
//
// Fully owned output prepared before return; caller must prepare/publish this
// entire value after success, not independently update civil/provenance fields.
// Qt/preparation allocation failures propagate without modifying caller inputs
// or previously published state. The bridge has no mutable/global state, clock
// reads, QtWidgets/UI, callbacks/reentrancy/event pumping, async/lifetime tokens,
// zone mapper/IANA/DST scheduler, persistence, source-accuracy claims or defaults.
// Future root native manual controls choose their own explicit caller policy;
// this bridge does not introduce a numeric bound or modify existing settings.
DeClockDisplaySnapshot make_de_clock_display_snapshot(
    const DeClockConfigurationSnapshot&, std::optional<UtcSeconds> actual_source,
    SyncStatus actual_synchronization);
} // namespace hamclock
