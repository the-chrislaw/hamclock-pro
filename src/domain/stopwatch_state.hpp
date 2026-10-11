#pragma once

#include "clock_state.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace hamclock {

// Original bounded replacement behavior, not certified original timer parity.
constexpr std::size_t kStopwatchMaximumLaps = 64;
enum class StopwatchPhase { Idle, Running, Stopped };
enum class StopwatchCommand { Start, Stop, Resume, Reset, Lap };
enum class StopwatchCommandResult { Applied, Unchanged };
struct StopwatchLap {
    // One-based position and cumulative/interval elapsed durations. A repeated
    // injected instant may legitimately produce a zero-length lap interval.
    std::size_t number = 0;
    ElapsedNanoseconds total{0};
    ElapsedNanoseconds interval{0};
};
struct StopwatchSnapshot {
    StopwatchPhase phase = StopwatchPhase::Idle;
    ElapsedNanoseconds elapsed{0};
    std::array<StopwatchLap, kStopwatchMaximumLaps> laps{};
    std::size_t lap_count = 0;
};
struct StopwatchTransition {
    StopwatchCommandResult result;
    StopwatchSnapshot snapshot;
};

// Pure injected monotonic state, no wall clock, scheduler, display, persistence,
// alarms, countdown or suspend detection. Time that the caller's monotonic
// clock advances while Running counts; a clock that freezes during suspend
// cannot be corrected here. Fixed 64-lap storage, no heap allocation on
// successful transitions; constructing standard exceptions may allocate.
class StopwatchState {
public:
    explicit StopwatchState(MonotonicNanoseconds initial_monotonic) noexcept;
    // Borrowed view of current state; later successful operations update it.
    const StopwatchSnapshot& snapshot() const noexcept;
    MonotonicNanoseconds last_observed() const noexcept;
    // Advances elapsed only while Running. Stopped/Idle elapsed stays frozen;
    // successful repeated instants are allowed. Every success records now.
    StopwatchSnapshot advance(MonotonicNanoseconds now);
    // Start: Idle -> Running; Running is Unchanged; Stopped requires Resume.
    // Stop: Running -> Stopped; Idle/Stopped are Unchanged.
    // Resume: Stopped -> Running, excluding all stopped intervals; Running is
    // Unchanged; Idle rejects. Lap: only Running; append without stopping.
    // Reset: any phase -> Idle, elapsed/laps zeroed; Idle is Unchanged.
    // Reset validates clock order but need not compute discarded elapsed, so
    // it can recover from an otherwise unrepresentable Running interval.
    // Repeated Start/Stop/Resume still advance an existing Running elapsed;
    // Unchanged describes command effect, not a freeze of injected time.
    // Invalid enum/reversed now throws invalid_argument; wrong phase throws
    // logic_error; lap capacity throws length_error; unrepresentable duration
    // throws overflow_error. ANY failure preserves the entire state including
    // last_observed. No partial time advance on rejected commands.
    StopwatchTransition apply(StopwatchCommand, MonotonicNanoseconds now);
private:
    MonotonicNanoseconds last_;
    StopwatchSnapshot snapshot_;
};

} // namespace hamclock
