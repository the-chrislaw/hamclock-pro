#pragma once
#include <cstdint>
#include <optional>

namespace hamclock {

// Separate signed64 units; no implicit conversion or mixed-clock arithmetic.
struct UtcSeconds { std::int64_t value; };
struct MonotonicNanoseconds { std::int64_t value; };
struct UtcOffsetSeconds { std::int64_t value; };
struct ElapsedNanoseconds { std::int64_t value; };

enum class SyncStatus { Unknown, Synchronized, Unsynchronized };
struct SyncEvidence {
    SyncStatus status = SyncStatus::Unknown;
    MonotonicNanoseconds observed_at{0};
};
struct SyncFreshnessPolicy {
    // Explicit caller policy; zero permits evidence only at the exact instant.
    ElapsedNanoseconds maximum_age{0};
};
struct ClockSnapshot {
    UtcSeconds actual_utc;
    UtcSeconds display_utc;
    bool preview = false;
    ElapsedNanoseconds elapsed;
    SyncStatus synchronization = SyncStatus::Unknown;
};

// Pure offline state: caller injects both clocks and optional quality evidence.
// UTC seconds use a caller-defined Unix/POSIX-second convention; this model
// does not format civil dates, represent leap seconds, synchronize host time,
// implement timers/alarms, or establish the accuracy of injected evidence.
class ClockState {
public:
    // Mandatory actual UTC is an available injected POSIX instant, including
    // valid epoch zero; Unknown refers only to synchronization evidence.
    // Missing/unknown source time needs a later source-validity integration.
    // Negative maximum_age throws std::invalid_argument.
    ClockState(UtcSeconds initial_utc, MonotonicNanoseconds initial_monotonic,
               SyncFreshnessPolicy policy);
    const ClockSnapshot& snapshot() const noexcept;
    // Updates atomically. UTC may jump either direction. Monotonic time may
    // repeat but never reverse; elapsed duration starts at construction.
    // Missing/Unknown, future, stale or unrepresentably old evidence => Unknown.
    // Fresh Unsynchronized remains explicitly Unsynchronized, never Healthy.
    // Throws invalid_argument for reversed monotonic/invalid evidence status;
    // overflow_error for unrepresentable elapsed/display; leaves state intact.
    ClockSnapshot advance(UtcSeconds actual_utc, MonotonicNanoseconds monotonic,
                          std::optional<SyncEvidence> evidence = std::nullopt);
    // Explicit preview, including a zero offset. Validates current display
    // addition before mutation; future advance also checks overflow.
    // Preview never changes actual UTC, elapsed time or synchronization.
    void set_preview_offset(UtcOffsetSeconds offset);
    void reset_preview() noexcept;
private:
    MonotonicNanoseconds origin_;
    MonotonicNanoseconds last_;
    SyncFreshnessPolicy policy_;
    UtcOffsetSeconds offset_{0};
    ClockSnapshot snapshot_;
};

} // namespace hamclock
