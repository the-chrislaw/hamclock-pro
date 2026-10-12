#pragma once

#include "clock_state.hpp"
#include <cstdint>

namespace hamclock {
// Documented original minimum is one minute. Seven-day maximum is an explicit
// replacement policy, not an original limit. Injected duration remains fixed
// for this object's lifetime; changing configuration creates a new service.
constexpr std::int64_t kCountdownMinimumNanoseconds = 60000000000;
constexpr std::int64_t kCountdownMaximumNanoseconds = 604800000000000;
enum class CountdownPhase { Idle, Running, Paused, Expired };
enum class CountdownCommand { Start, Stop, Resume, Reset };
enum class CountdownCommandResult { Applied, Unchanged };
struct CountdownSnapshot {
    CountdownPhase phase = CountdownPhase::Idle;
    ElapsedNanoseconds configured{0};
    ElapsedNanoseconds remaining{0};
    // Instance-scoped run identity, zero before first Start; Start allocates
    // next generation. Reset preserves it. Caller must bind tokens to this
    // service instance; separate/new objects may reuse IDs. No persistence IDs.
    std::uint64_t run_id = 0;
    // Pending belongs to run_id. Acknowledged expiry cannot re-arm this run.
    // No external alarm or delivered-notice guarantee.
    bool expiry_pending = false;
};
struct CountdownTransition {
    CountdownCommandResult result;
    CountdownSnapshot snapshot;
};
// Pure checked increment used by every Start before commit. Full uint64 range;
// maximum throws overflow_error rather than wrapping/reusing an identity.
std::uint64_t next_countdown_run_id(std::uint64_t previous);

// Original pure injected monotonic countdown, no clock reads, Qt, alarm sound,
// wall time, scheduling, persistence, suspend correction or radio interaction.
// Constructor validates inclusive one-minute..seven-day configured duration;
// zero/negative/out-of-range throws invalid_argument. Initial monotonic may be
// any signed64 instant. Duration types are unit-tagged nanoseconds, not UTC.
// Start always starts/restarts the full configured duration from supplied now,
// clears pending expiry, allocates next run_id and returns Applied (even already
// Running). Exhausted identities reject atomically. Stop while
// Running first advances: pauses if remaining>0, otherwise remains Expired;
// Stop in other phases is Unchanged. Resume only Paused; Idle/Expired reject,
// Running is Unchanged but advances. Reset any phase -> Idle/full duration/no
// pending, retaining run_id; Idle is Unchanged. acknowledge(id,now) rejects zero
// or mismatching identity atomically, so stale tokens cannot clear new expiry.
// Matching identity advances then clears pending without changing phase;
// returns Applied if pending was cleared, otherwise Unchanged.
// advance subtracts only Running time, clamps remaining at zero on/beyond expiry
// and sets Expired/pending once. Paused time never counts. Large ordered jumps
// (including full signed64 span) safely saturate rather than overflow or wrap.
// Repeat instants allowed; every success records last_observed, including noops.
// Invalid enum/reversed time/duration/ack ID throws invalid_argument; wrong Resume
// phase throws logic_error. ANY failure preserves phase/configuration/remaining/run_id/
// pending and last_observed. Explicit Start/Reset discard elapsed intervals but
// still validate time order. Successful operations allocate no heap storage;
// standard exception construction may allocate. No full parity claim.
class CountdownState {
public:
    CountdownState(ElapsedNanoseconds configured, MonotonicNanoseconds initial);
    const CountdownSnapshot& snapshot() const noexcept;
    MonotonicNanoseconds last_observed() const noexcept;
    CountdownSnapshot advance(MonotonicNanoseconds now);
    CountdownTransition apply(CountdownCommand, MonotonicNanoseconds now);
    CountdownTransition acknowledge(std::uint64_t run_id, MonotonicNanoseconds now);
private:
    MonotonicNanoseconds last_;
    CountdownSnapshot snapshot_;
};
} // namespace hamclock
