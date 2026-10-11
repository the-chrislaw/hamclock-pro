#pragma once

#include "../domain/stopwatch_state.hpp"
#include <cstddef>
#include <string>

namespace hamclock {

constexpr std::size_t kStopwatchMaximumFormattedBytes = 16;
// Original formatter: truncate nonnegative nanoseconds to centiseconds; format
// HH:MM:SS.cc with hours at least two digits, unbounded within signed64 input
// (at most seven hour digits). No wrap at 24h or rounding upward. Output owns
// its bytes. Negative duration throws invalid_argument; allocation may throw.
std::string format_stopwatch_duration(ElapsedNanoseconds);

enum class StopwatchDisplayCommand { Hold, Release };
enum class StopwatchDisplayResult { Applied, Unchanged };
enum class StopwatchDisplayLabel { Idle, Running, RunningHeld, Stopped };
struct StopwatchPresentationView {
    StopwatchPhase phase = StopwatchPhase::Idle;
    StopwatchDisplayLabel label = StopwatchDisplayLabel::Idle;
    bool held = false;
    ElapsedNanoseconds actual_elapsed{0};
    ElapsedNanoseconds displayed_elapsed{0};
    std::string formatted = "00:00:00.00";
    std::size_t lap_count = 0;
};
struct StopwatchDisplayTransition {
    StopwatchDisplayResult result;
    StopwatchPresentationView view;
};

// Validates phase, nonnegative duration, lap_count<=64; active laps have exact
// one-based numbering, nondecreasing totals <= elapsed and intervals matching
// cumulative differences. Unused lap slots must be zero. Idle requires zero
// elapsed/no laps. Throws invalid_argument. No original-parity/default claim.
void validate_stopwatch_snapshot(const StopwatchSnapshot&);

// Pure display latch, never modifies the underlying stopwatch. All inputs and
// returned views are owned values. Incoming snapshots are validated every time;
// calling code must keep advancing the shared domain clock while held.
// Replacement policies: Hold is Running-only; first Hold captures current
// elapsed. Repeated Hold preserves the original held value, result Unchanged,
// while actual_elapsed/lap_count continue updating. Explicit named Release
// returns the currently supplied elapsed, even if no hold is active (Unchanged).
// Stop or Idle input clears hold automatically and shows supplied elapsed;
// Idle reflects a domain reset. No undocumented original unfreeze gesture.
// While held, a Running elapsed below stored displayed_elapsed rejects (reset
// requires an Idle snapshot or Release); this avoids retaining a held display
// greater than actual time.
// Other transitions are validated snapshots, not proof of intervening history.
// Invalid command, wrong-phase Hold, invalid snapshot or duration rejects with
// no change to ANY state. All copies/formatting complete before nonthrowing
// commit; allocation failures preserve prior state too. No timers/Qt/IO.
class StopwatchPresentation {
public:
    StopwatchPresentation() = default;
    // Owned copy; caller mutation/lifetime cannot alter stored presentation.
    StopwatchPresentationView view() const;
    StopwatchPresentationView refresh(const StopwatchSnapshot&);
    StopwatchDisplayTransition apply(StopwatchDisplayCommand,
                                     const StopwatchSnapshot&);
private:
    StopwatchPresentationView view_;
};

} // namespace hamclock
