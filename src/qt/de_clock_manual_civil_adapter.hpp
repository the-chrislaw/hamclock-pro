#pragma once

#include "../app/de_clock_configuration.hpp"
#include "big_clock_civil_adapter.hpp"

namespace hamclock {
// Unix-shaped calendar encoding after a manual offset; NOT an actual UTC
// instant or source time. Kept distinct from UtcSeconds in public provenance.
struct DeClockShiftedCivilSeconds { std::int64_t value; };
enum class DeClockManualCivilStatus {
    Ready, MissingSource, AutomaticUnresolved, SourceOutOfRange,
    OffsetOverflow, DisplayOutOfRange, InvalidConversion
};
struct DeClockManualCivilResult {
    DeClockManualCivilStatus status;
    DeClockMode desired_mode;
    // Actual injection preserved exactly, including out-of-range raw evidence.
    // Absence is missing source; epoch0 is present and can be valid.
    std::optional<UtcSeconds> actual_source;
    // Copied manual provenance iff ManualOffset; no inferred correction/DST.
    std::optional<UtcOffsetSeconds> manual_offset;
    // Present after successful checked addition even if display is out of
    // product range; absent for missing/automatic/source-range/overflow failures.
    std::optional<DeClockShiftedCivilSeconds> shifted_calendar;
    // Owned local civil fields iff Ready; interpreted as fixed-offset DE time,
    // not relabeled actual UTC, resolved IANA zone or source accuracy proof.
    std::optional<BigClockCivilTime> civil;
};

// Pure fixed manual-offset conversion, no clock read, mutable state, mapper,
// host-local inference, IANA/DST rules, scheduling, QtWidgets, UI, IO/persistence
// or revision/token authority. Caller context binding remains a separate gate.
// Validate complete configuration AND caller bounds first, even if source
// missing/Automatic; invalid config throws invalid_argument without mutation.
// Status precedence after validation: absent source -> MissingSource (mode and
// supplied manual provenance retained); present Automatic -> AutomaticUnresolved
// with no conversion/fallback; Manual actual outside UTC adapter's inclusive
// year1..9999 seconds range -> SourceOutOfRange before addition or any Qt call.
// Then checked signed64 actual+offset: overflow -> OffsetOverflow/no shifted
// value, never wrap. Representable sum is retained with its distinct unit;
// outside same year1..9999 display range -> DisplayOutOfRange before Qt call.
// Both actual and shifted product bounds are explicit replacement policy,
// consistent with the guarded UTC adapter, not original HamClock numeric limits.
// Only bounded sum goes to reviewed UTC-shaped calendar conversion; verify its
// Ready/owned civil result before exposing fixed-offset local fields. Unexpected
// conversion failure -> InvalidConversion/no civil, no fabricated midnight.
// Qt/preparation allocation exceptions may propagate; inputs remain unchanged.
// No leap-second second60, sync/freshness inference or actual/preview selection.
DeClockManualCivilResult adapt_de_clock_manual_civil(
    const DeClockConfiguration&, const DeClockOffsetPolicy&, std::optional<UtcSeconds> actual);
} // namespace hamclock
