#pragma once

#include "../domain/countdown_state.hpp"
#include <cstddef>
#include <string>

namespace hamclock {
constexpr std::size_t kCountdownMaximumFormattedBytes = 9;
// Replacement display policy: ceil nonnegative nanoseconds to seconds by
// quotient/remainder, so positive remaining never appears expired early.
// Format HH:MM:SS, minimum two hour digits, no wrap at 24h, maximum168:00:00.
// Accept zero..seven days inclusive, reject negative/above cap invalid_argument.
// The same ceiling is used for configured display; exact raw durations remain
// available in the view. This rounding is not documented original behavior.
std::string format_countdown_duration(ElapsedNanoseconds);

// Require known phase, configured within inclusive model min/max, remaining
// within0..configured. Idle: remaining==configured, pendingfalse (run_id may
// retain prior generations). Running/Paused: run_id>0, remaining>0, pendingfalse.
// Expired: run_id>0, remaining0; pending true or acknowledgedfalse. Reject any
// violation invalid_argument. Validation is structural, not proof of history
// or service lifetime identity; no acknowledgement authority is minted here.
void validate_countdown_snapshot(const CountdownSnapshot&);

struct CountdownPresentationView {
    CountdownPhase phase;
    ElapsedNanoseconds configured;
    ElapsedNanoseconds remaining;
    std::uint64_t run_id;
    bool expiry_pending;
    // Pure control affordance metadata: caller still binds service lifetime
    // plus captured run_id before an acknowledgment. No token created here.
    bool dismiss_available;
    std::string configured_text;
    std::string remaining_text;
    // Ready / Running / Paused / Time elapsed / Time elapsed - alert pending.
    // Pending clear stays Expired and zero, never silently starts/resets.
    std::string status;
};

// Pure stateless owned conversion, no clock reads/Qt/timer/network/IO/persistence.
// Validate complete immutable snapshot before formatting; initialize every view
// field explicitly and return fully owned values. Invalid snapshot/preparation
// allocation failure never mutates caller snapshot or existing view; calling
// service must prepare this return before committing its own state changes.
// No history/lifetime proof across conversions. No hidden timer activation.
CountdownPresentationView make_countdown_presentation(const CountdownSnapshot&);
} // namespace hamclock
