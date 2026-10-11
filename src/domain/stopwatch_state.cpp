#include "stopwatch_state.hpp"

#include <limits>
#include <stdexcept>

namespace hamclock {
namespace {
bool valid_command(StopwatchCommand command) noexcept {
    switch (command) {
    case StopwatchCommand::Start:
    case StopwatchCommand::Stop:
    case StopwatchCommand::Resume:
    case StopwatchCommand::Reset:
    case StopwatchCommand::Lap:
        return true;
    }
    return false;
}

void validate_order(MonotonicNanoseconds now, MonotonicNanoseconds last) {
    if (now.value < last.value) {
        throw std::invalid_argument("reversed stopwatch monotonic time");
    }
}

StopwatchSnapshot advanced(const StopwatchSnapshot& previous,
                           MonotonicNanoseconds last,
                           MonotonicNanoseconds now) {
    validate_order(now, last);
    auto candidate = previous;
    if (previous.phase == StopwatchPhase::Running) {
        // Valid ordering permits unsigned subtraction over arbitrary signed64
        // endpoints. Compare against remaining capacity before any signed cast.
        const auto increment = static_cast<std::uint64_t>(now.value) -
                               static_cast<std::uint64_t>(last.value);
        const auto remaining = static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max() - previous.elapsed.value);
        if (increment > remaining) {
            throw std::overflow_error("stopwatch elapsed duration overflow");
        }
        candidate.elapsed.value += static_cast<std::int64_t>(increment);
    }
    return candidate;
}
} // namespace

StopwatchState::StopwatchState(MonotonicNanoseconds initial_monotonic) noexcept
    : last_(initial_monotonic) {}

const StopwatchSnapshot& StopwatchState::snapshot() const noexcept {
    return snapshot_;
}

MonotonicNanoseconds StopwatchState::last_observed() const noexcept {
    return last_;
}

StopwatchSnapshot StopwatchState::advance(MonotonicNanoseconds now) {
    const auto candidate = advanced(snapshot_, last_, now);
    snapshot_ = candidate;
    last_ = now;
    return snapshot_;
}

StopwatchTransition StopwatchState::apply(StopwatchCommand command,
                                         MonotonicNanoseconds now) {
    if (!valid_command(command)) {
        throw std::invalid_argument("unknown stopwatch command");
    }
    validate_order(now, last_);

    if (command == StopwatchCommand::Reset) {
        const auto result = snapshot_.phase == StopwatchPhase::Idle
            ? StopwatchCommandResult::Unchanged
            : StopwatchCommandResult::Applied;
        // No elapsed calculation is necessary for explicitly discarded data.
        snapshot_ = StopwatchSnapshot{};
        last_ = now;
        return {result, snapshot_};
    }
    if (command == StopwatchCommand::Start &&
        snapshot_.phase == StopwatchPhase::Stopped) {
        throw std::logic_error("stopped stopwatch requires resume");
    }
    if (command == StopwatchCommand::Resume &&
        snapshot_.phase == StopwatchPhase::Idle) {
        throw std::logic_error("idle stopwatch cannot resume");
    }
    if (command == StopwatchCommand::Lap) {
        if (snapshot_.phase != StopwatchPhase::Running) {
            throw std::logic_error("lap requires running stopwatch");
        }
        if (snapshot_.lap_count == kStopwatchMaximumLaps) {
            throw std::length_error("stopwatch lap capacity reached");
        }
    }

    auto candidate = advanced(snapshot_, last_, now);
    auto result = StopwatchCommandResult::Unchanged;
    switch (command) {
    case StopwatchCommand::Start:
        if (candidate.phase == StopwatchPhase::Idle) {
            candidate.phase = StopwatchPhase::Running;
            result = StopwatchCommandResult::Applied;
        }
        break;
    case StopwatchCommand::Stop:
        if (candidate.phase == StopwatchPhase::Running) {
            candidate.phase = StopwatchPhase::Stopped;
            result = StopwatchCommandResult::Applied;
        }
        break;
    case StopwatchCommand::Resume:
        if (candidate.phase == StopwatchPhase::Stopped) {
            candidate.phase = StopwatchPhase::Running;
            result = StopwatchCommandResult::Applied;
        }
        break;
    case StopwatchCommand::Lap: {
        const auto previous_total = candidate.lap_count == 0
            ? std::int64_t{0}
            : candidate.laps[candidate.lap_count - 1].total.value;
        candidate.laps[candidate.lap_count] = {
            candidate.lap_count + 1, candidate.elapsed,
            {candidate.elapsed.value - previous_total}
        };
        ++candidate.lap_count;
        result = StopwatchCommandResult::Applied;
        break;
    }
    case StopwatchCommand::Reset:
        // Handled above because reset need not calculate discarded elapsed.
        break;
    }
    snapshot_ = candidate;
    last_ = now;
    return {result, snapshot_};
}
} // namespace hamclock
