#include "countdown_state.hpp"
#include <limits>
#include <stdexcept>

namespace hamclock {
namespace {
void validate_order(MonotonicNanoseconds now, MonotonicNanoseconds last) {
    if (now.value < last.value) throw std::invalid_argument("Reversed countdown monotonic time");
}
CountdownSnapshot advanced(const CountdownSnapshot& previous,
                           MonotonicNanoseconds last, MonotonicNanoseconds now) {
    validate_order(now, last);
    auto candidate = previous;
    if (candidate.phase == CountdownPhase::Running) {
        // Ordered signed endpoints subtract safely over the full unsigned span.
        const auto increment = static_cast<std::uint64_t>(now.value) -
                               static_cast<std::uint64_t>(last.value);
        if (increment >= static_cast<std::uint64_t>(candidate.remaining.value)) {
            candidate.remaining.value = 0;
            candidate.phase = CountdownPhase::Expired;
            candidate.expiry_pending = true;
        } else {
            candidate.remaining.value -= static_cast<std::int64_t>(increment);
        }
    }
    return candidate;
}
bool valid(CountdownCommand command) noexcept {
    switch (command) {
    case CountdownCommand::Start:
    case CountdownCommand::Stop:
    case CountdownCommand::Resume:
    case CountdownCommand::Reset: return true;
    }
    return false;
}
} // namespace

std::uint64_t next_countdown_run_id(std::uint64_t previous) {
    if (previous == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Countdown run identities exhausted");
    }
    return previous + 1;
}

CountdownState::CountdownState(ElapsedNanoseconds configured, MonotonicNanoseconds initial)
    : last_(initial) {
    if (configured.value < kCountdownMinimumNanoseconds || configured.value > kCountdownMaximumNanoseconds) {
        throw std::invalid_argument("Countdown duration must be one minute through seven days");
    }
    snapshot_.configured = configured;
    snapshot_.remaining = configured;
}
const CountdownSnapshot& CountdownState::snapshot() const noexcept { return snapshot_; }
MonotonicNanoseconds CountdownState::last_observed() const noexcept { return last_; }
CountdownSnapshot CountdownState::advance(MonotonicNanoseconds now) {
    const auto candidate = advanced(snapshot_, last_, now);
    snapshot_ = candidate;
    last_ = now;
    return snapshot_;
}
CountdownTransition CountdownState::apply(CountdownCommand command, MonotonicNanoseconds now) {
    if (!valid(command)) throw std::invalid_argument("Unknown countdown command");
    validate_order(now, last_);
    auto result = CountdownCommandResult::Unchanged;
    auto candidate = snapshot_;
    if (command == CountdownCommand::Start || command == CountdownCommand::Reset) {
        if (command == CountdownCommand::Start) {
            candidate.run_id = next_countdown_run_id(candidate.run_id);
            candidate.phase = CountdownPhase::Running;
            result = CountdownCommandResult::Applied;
        } else {
            if (candidate.phase != CountdownPhase::Idle) result = CountdownCommandResult::Applied;
            candidate.phase = CountdownPhase::Idle;
        }
        candidate.remaining = candidate.configured;
        candidate.expiry_pending = false;
    } else {
        if (command == CountdownCommand::Resume && candidate.phase != CountdownPhase::Running &&
            candidate.phase != CountdownPhase::Paused) {
            throw std::logic_error("Countdown resume requires a paused or running countdown");
        }
        candidate = advanced(snapshot_, last_, now);
        if (command == CountdownCommand::Stop && snapshot_.phase == CountdownPhase::Running) {
            if (candidate.phase == CountdownPhase::Running) candidate.phase = CountdownPhase::Paused;
            result = CountdownCommandResult::Applied;
        } else if (command == CountdownCommand::Resume && candidate.phase == CountdownPhase::Paused) {
            candidate.phase = CountdownPhase::Running;
            result = CountdownCommandResult::Applied;
        }
    }
    snapshot_ = candidate;
    last_ = now;
    return {result, snapshot_};
}
CountdownTransition CountdownState::acknowledge(std::uint64_t id, MonotonicNanoseconds now) {
    if (id == 0 || id != snapshot_.run_id) throw std::invalid_argument("Unknown countdown run identity");
    auto candidate = advanced(snapshot_, last_, now);
    const auto result = candidate.expiry_pending ? CountdownCommandResult::Applied
                                                  : CountdownCommandResult::Unchanged;
    candidate.expiry_pending = false;
    snapshot_ = candidate;
    last_ = now;
    return {result, snapshot_};
}
} // namespace hamclock
