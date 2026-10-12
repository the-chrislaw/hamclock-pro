#include "stopwatch_presentation.hpp"
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hamclock {
namespace {
static_assert(std::is_nothrow_swappable_v<StopwatchPresentationView>);
static_assert(std::is_nothrow_move_constructible_v<StopwatchPresentationView>);
static_assert(std::is_nothrow_move_constructible_v<StopwatchDisplayTransition>);

std::string two_digits(std::int64_t value) {
    std::string output(2, '0');
    output[0] = static_cast<char>('0' + value / 10);
    output[1] = static_cast<char>('0' + value % 10);
    return output;
}

StopwatchPresentationView prepare(const StopwatchPresentationView& previous,
                                  const StopwatchSnapshot& snapshot,
                                  bool release, bool hold) {
    validate_stopwatch_snapshot(snapshot);
    if (hold && snapshot.phase != StopwatchPhase::Running) {
        throw std::invalid_argument("Hold requires a running stopwatch");
    }
    auto candidate = previous;
    candidate.phase = snapshot.phase;
    candidate.actual_elapsed = snapshot.elapsed;
    candidate.lap_count = snapshot.lap_count;
    if (release || snapshot.phase != StopwatchPhase::Running) {
        candidate.held = false;
        candidate.displayed_elapsed = snapshot.elapsed;
    } else if (previous.held) {
        if (snapshot.elapsed.value < previous.displayed_elapsed.value) {
            throw std::invalid_argument("Held display exceeds supplied actual elapsed");
        }
    } else {
        candidate.displayed_elapsed = snapshot.elapsed;
        candidate.held = hold;
    }
    switch (candidate.phase) {
    case StopwatchPhase::Idle:
        candidate.label = StopwatchDisplayLabel::Idle;
        break;
    case StopwatchPhase::Running:
        candidate.label = candidate.held ? StopwatchDisplayLabel::RunningHeld
                                         : StopwatchDisplayLabel::Running;
        break;
    case StopwatchPhase::Stopped:
        candidate.label = StopwatchDisplayLabel::Stopped;
        break;
    }
    candidate.formatted = format_stopwatch_duration(candidate.displayed_elapsed);
    return candidate;
}
} // namespace

std::string format_stopwatch_duration(ElapsedNanoseconds duration) {
    if (duration.value < 0) {
        throw std::invalid_argument("Stopwatch duration must be nonnegative");
    }
    const auto centiseconds = duration.value / 10000000;
    const auto seconds = centiseconds / 100;
    const auto minutes = seconds / 60;
    auto hours = std::to_string(minutes / 60);
    if (hours.size() < 2) {
        hours.insert(0, 1, '0');
    }
    return hours + ':' + two_digits(minutes % 60) + ':' +
           two_digits(seconds % 60) + '.' + two_digits(centiseconds % 100);
}

void validate_stopwatch_snapshot(const StopwatchSnapshot& snapshot) {
    switch (snapshot.phase) {
    case StopwatchPhase::Idle:
    case StopwatchPhase::Running:
    case StopwatchPhase::Stopped:
        break;
    default:
        throw std::invalid_argument("Unknown stopwatch phase");
    }
    if (snapshot.elapsed.value < 0 || snapshot.lap_count > kStopwatchMaximumLaps) {
        throw std::invalid_argument("Invalid stopwatch duration or lap count");
    }
    if (snapshot.phase == StopwatchPhase::Idle &&
        (snapshot.elapsed.value != 0 || snapshot.lap_count != 0)) {
        throw std::invalid_argument("Idle stopwatch must be reset");
    }
    std::int64_t previous_total = 0;
    for (std::size_t i = 0; i < snapshot.laps.size(); ++i) {
        const auto& lap = snapshot.laps[i];
        if (i >= snapshot.lap_count) {
            if (lap.number != 0 || lap.total.value != 0 || lap.interval.value != 0) {
                throw std::invalid_argument("Unused lap storage must be zero");
            }
            continue;
        }
        if (lap.number != i + 1 || lap.total.value < previous_total ||
            lap.total.value > snapshot.elapsed.value ||
            lap.interval.value != lap.total.value - previous_total) {
            throw std::invalid_argument("Invalid stopwatch lap");
        }
        previous_total = lap.total.value;
    }
}

StopwatchPresentationView StopwatchPresentation::view() const {
    return view_;
}

StopwatchPresentationView StopwatchPresentation::refresh(const StopwatchSnapshot& snapshot) {
    auto candidate = prepare(view_, snapshot, false, false);
    auto output = candidate;
    using std::swap;
    swap(view_, candidate);
    return output;
}

StopwatchDisplayTransition StopwatchPresentation::apply(
    StopwatchDisplayCommand command, const StopwatchSnapshot& snapshot) {
    if (command != StopwatchDisplayCommand::Hold && command != StopwatchDisplayCommand::Release) {
        throw std::invalid_argument("Unknown stopwatch display command");
    }
    const bool hold = command == StopwatchDisplayCommand::Hold;
    auto candidate = prepare(view_, snapshot, !hold, hold);
    const auto result = hold == view_.held ? StopwatchDisplayResult::Unchanged
                                         : StopwatchDisplayResult::Applied;
    StopwatchDisplayTransition output{result, candidate};
    using std::swap;
    swap(view_, candidate);
    return output;
}
} // namespace hamclock
