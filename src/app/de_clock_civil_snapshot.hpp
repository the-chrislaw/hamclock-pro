#pragma once

#include "de_clock_configuration.hpp"
#include <optional>

namespace hamclock {
// Minimal proposed synchronous context guard, NOT another clock/configuration
// service or a civil converter/cache. Existing stable configuration state and
// reviewed UTC/manual adapter already own those responsibilities.
struct DeClockCivilContext {
    // Owned full desired settings/bounds/canonical location/revisions captured
    // from one existing configuration session before conversion begins.
    DeClockConfigurationSnapshot station;
    // ACTUAL source only. Do not substitute preview UTC or a manual adapter's
    // shifted-calendar seconds here; shifted provenance stays in adapter result.
    std::optional<UtcSeconds> actual_source;
};
enum class DeClockCivilContextStatus { Matched, StaleContext };
struct DeClockCivilContextBinding {
    DeClockCivilContextStatus status;
    // Present iff Matched; owned context for pairing with the original adapter
    // result. StaleContext has no accepted context and must not display civil
    // values as current station data. Existing raw result remains inspectable
    // separately; do not rewrite its actual/shifted/manual provenance.
    std::optional<DeClockCivilContext> accepted;
};

// Validate complete config/policy and finite/range-valid location plus positive
// configuration/location revisions. Require stored canonical longitude[-180,180)
// (existing state snapshots canonicalize+180); signedzero equality allowed.
// No source means unavailable source, not epochzero or Unknown synchronization.
// Source may be extreme raw evidence: range/status belongs to reviewed adapter.
// Invalid schema throws invalid_argument even when comparison would be stale.
void validate_de_clock_civil_context(const DeClockCivilContext&);

// Validate BOTH full contexts before comparison. Match every desired mode/
// manual-offset payload, caller policy bound, exact canonical coordinates,
// configuration/location revision and optional actual-source presence/value.
// Any valid difference -> StaleContext/no accepted; exact equality -> Matched
// with owned completed context, allocation-free on success. No epsilon/nudge,
// partial field match, fallback/time conversion or inference from unavailable
// Automatic/missing time. Matched means context agreement only, never Ready,
// resolved automatic zone, synchronization/currentness or valid civil payload.
//
// Caller must use the same stable owning session and pair completed context
// captured for the actual adapter invocation with its unchanged result. This
// function does not establish session lifetime/authenticity, prevent forged
// contexts, or validate civil payload/provenance consistency. An independent
// synchronous adapter bridge must additionally check result.actual_source/mode/
// manual offset against the captured context before supplying DE civil fields.
// Identical values/counters in a different session are not global authority.
// No async callbacks/tokens, global IDs, disk persistence, Qt, mapper, provider,
// actual clock reads, scheduling, shared mutable state or service activation.
DeClockCivilContextBinding bind_de_clock_civil_context(
    const DeClockCivilContext& current, const DeClockCivilContext& completed);
} // namespace hamclock
