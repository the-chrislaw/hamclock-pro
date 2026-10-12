#pragma once

#include "../app/big_clock_presentation.hpp"
#include <optional>

namespace hamclock {
// Proposed four-digit proleptic Gregorian product bounds, independently
// verified from literal civil dates. Not Qt's full supported range or original
// HamClock bounds. Precheck BEFORE Qt conversion: tagged6.4 QDoc describes
// undefined behavior for out-of-range fromSecsSinceEpoch arguments.
constexpr std::int64_t kBigClockUtcCivilMinimumSeconds = -62135596800;
constexpr std::int64_t kBigClockUtcCivilMaximumSeconds = 253402300799;
enum class BigClockUtcCivilStatus { Ready, MissingSource, OutOfRange, InvalidConversion };
struct BigClockUtcCivilResult {
    BigClockUtcCivilStatus status;
    // Original injection retained even when out of product range; absent only
    // for missing source. Epoch zero is present and valid, never a sentinel.
    std::optional<UtcSeconds> source;
    // Present iff Ready; owned validated year1..9999/time0..23:0..59:0..59.
    std::optional<BigClockCivilTime> civil;
};

// Pure UTC-only adapter: no system clock, synchronization inference, preview
// selection/arithmetic, timezone mapper/DE, DST scheduling, local-time fallback,
// source accuracy/freshness guarantee, settings, UI, IO or global state.
// Missing input => MissingSource/no civil. Below/above inclusive product bounds
// => OutOfRange/no civil WITHOUT invoking Qt on that value. For in-range input,
// explicitly use fromSecsSinceEpoch(...,Qt::UTC) on Qt<6.9; on Qt>=6.9 use
// its equivalent QTimeZone::UTC overload (lightweight initializer since6.5),
// avoiding the TimeSpec overload's since6.9 deprecation. Compile-time version
// selection, no global warning suppression and no implicit/local zone/backend.
// Check datetime/date/
// time validity and validate complete returned civil fields with public CB1a
// contract. Invalid Qt output => InvalidConversion/no fabricated midnight.
// Ready retains exactly injected POSIX instant and valid owned civil fields;
// POSIX/Qt convention has no second60 support. No silent date normalization.
// Qt/preparation allocation exceptions may propagate; function has no mutable
// state and never changes caller input. No claim of Qt6.4 executable/Pi evidence
// follows from an installed newer-Qt build; those are separate target gates.
BigClockUtcCivilResult adapt_big_clock_utc_civil(std::optional<UtcSeconds>);
} // namespace hamclock
