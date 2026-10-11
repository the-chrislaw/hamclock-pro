#include "../src/domain/stopwatch_state.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
int checks = 0;
int failures = 0;
void check(bool condition, const char* label) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << label << '\n';
    }
}
bool equal(const hamclock::StopwatchSnapshot& a,
           const hamclock::StopwatchSnapshot& b) {
    if (a.phase != b.phase || a.elapsed.value != b.elapsed.value ||
        a.lap_count != b.lap_count) {
        return false;
    }
    for (std::size_t i = 0; i < a.laps.size(); ++i) {
        if (a.laps[i].number != b.laps[i].number ||
            a.laps[i].total.value != b.laps[i].total.value ||
            a.laps[i].interval.value != b.laps[i].interval.value) {
            return false;
        }
    }
    return true;
}
template<typename Exception, typename Action>
void rejects(hamclock::StopwatchState& state, Action action, const char* label) {
    const auto before = state.snapshot();
    const auto last = state.last_observed();
    bool rejected = false;
    try {
        action();
    } catch (const Exception&) {
        rejected = true;
    }
    check(rejected, label);
    check(equal(state.snapshot(), before) && state.last_observed().value == last.value,
          "rejected operation preserves all elapsed/lap/phase/time state");
}
}

int main() {
    using namespace hamclock;
    StopwatchState state({0});
    check(state.snapshot().phase == StopwatchPhase::Idle &&
          state.snapshot().elapsed.value == 0 && state.snapshot().lap_count == 0,
          "initial idle zero is not running");
    check(state.advance({100}).elapsed.value == 0, "idle time is excluded");
    check(state.apply(StopwatchCommand::Stop, {100}).result == StopwatchCommandResult::Unchanged,
          "stop idle is harmless");
    rejects<std::logic_error>(state, [&] { state.apply(StopwatchCommand::Resume, {200}); },
                              "idle cannot resume an unstarted stopwatch");
    rejects<std::logic_error>(state, [&] { state.apply(StopwatchCommand::Lap, {200}); },
                              "idle cannot record lap");
    check(state.apply(StopwatchCommand::Start, {200}).result == StopwatchCommandResult::Applied,
          "start establishes running interval");
    check(state.advance({250}).elapsed.value == 50, "running accumulation");
    auto lap = state.apply(StopwatchCommand::Lap, {300}).snapshot;
    check(lap.phase == StopwatchPhase::Running && lap.elapsed.value == 100,
          "lap never stops running");
    check(lap.laps[0].number == 1 && lap.laps[0].total.value == 100 &&
          lap.laps[0].interval.value == 100, "first lap cumulative and interval");
    lap = state.apply(StopwatchCommand::Lap, {300}).snapshot;
    check(lap.laps[1].number == 2 && lap.laps[1].total.value == 100 &&
          lap.laps[1].interval.value == 0, "repeated instant produces explicit zero lap");
    check(state.apply(StopwatchCommand::Start, {400}).result == StopwatchCommandResult::Unchanged &&
          state.snapshot().elapsed.value == 200, "repeated start retains counting");
    check(state.apply(StopwatchCommand::Resume, {450}).result == StopwatchCommandResult::Unchanged &&
          state.snapshot().elapsed.value == 250, "repeated resume retains counting");
    check(state.apply(StopwatchCommand::Stop, {500}).snapshot.elapsed.value == 300,
          "stop counts interval through injected stop instant");
    check(state.advance({500000}).elapsed.value == 300, "long stopped pause is frozen");
    rejects<std::logic_error>(state, [&] { state.apply(StopwatchCommand::Start, {500100}); },
                              "stopped requires explicit resume");
    rejects<std::logic_error>(state, [&] { state.apply(StopwatchCommand::Lap, {500100}); },
                              "stopped lap rejected");
    check(state.apply(StopwatchCommand::Stop, {500100}).result == StopwatchCommandResult::Unchanged,
          "repeated stop advances injected time but not elapsed");
    check(state.apply(StopwatchCommand::Resume, {600000}).snapshot.elapsed.value == 300,
          "resume excludes entire stopped interval");
    lap = state.apply(StopwatchCommand::Lap, {600100}).snapshot;
    check(lap.elapsed.value == 400 && lap.laps[2].total.value == 400 &&
          lap.laps[2].interval.value == 300, "lap interval excludes stop but includes running since prior lap");
    rejects<std::invalid_argument>(state, [&] { state.advance({600099}); }, "reversed advance");
    rejects<std::invalid_argument>(state, [&] { state.apply(StopwatchCommand::Reset, {600099}); },
                                   "reset does not permit backwards clock");
    rejects<std::invalid_argument>(state, [&] { state.apply(static_cast<StopwatchCommand>(999), {600100}); },
                                   "invalid command rejected");
    auto reset = state.apply(StopwatchCommand::Reset, {600200});
    check(reset.result == StopwatchCommandResult::Applied &&
          reset.snapshot.phase == StopwatchPhase::Idle && reset.snapshot.elapsed.value == 0 &&
          reset.snapshot.lap_count == 0, "reset clears full run state");
    for (const auto& item : reset.snapshot.laps) {
        check(item.number == 0 && item.total.value == 0 && item.interval.value == 0,
              "reset clears unused and previous lap storage");
    }
    check(state.apply(StopwatchCommand::Reset, {600201}).result == StopwatchCommandResult::Unchanged,
          "repeat idle reset harmless");
    state.apply(StopwatchCommand::Start, {600201});
    for (std::size_t i = 0; i < kStopwatchMaximumLaps; ++i) {
        const auto value = state.apply(StopwatchCommand::Lap, {600202 + static_cast<std::int64_t>(i)});
        check(value.snapshot.lap_count == i + 1 && value.snapshot.laps[i].number == i + 1,
              "bounded lap append preserves order");
    }
    rejects<std::length_error>(state, [&] { state.apply(StopwatchCommand::Lap, {700000}); },
                               "lap one over cap rejects without partial time advance");
    check(state.advance({700000}).elapsed.value == 99799, "running continues after rejected lap");
    state.apply(StopwatchCommand::Reset, {700000});
    state.apply(StopwatchCommand::Start, {700000});
    check(state.apply(StopwatchCommand::Lap, {700000}).snapshot.laps[0].number == 1,
          "reset restarts lap numbering and zero duration valid");

    constexpr auto low = std::numeric_limits<std::int64_t>::min();
    constexpr auto high = std::numeric_limits<std::int64_t>::max();
    StopwatchState limit({low});
    limit.apply(StopwatchCommand::Start, {low});
    check(limit.advance({-1}).elapsed.value == high, "exact signed maximum elapsed representable");
    rejects<std::overflow_error>(limit, [&] { limit.advance({0}); }, "accumulation one over max rejects");
    check(limit.apply(StopwatchCommand::Stop, {-1}).snapshot.elapsed.value == high,
          "stop at accepted boundary remains possible");
    check(limit.advance({high}).elapsed.value == high, "stopped full signed span does not subtract pause");
    limit.apply(StopwatchCommand::Resume, {high});
    check(limit.advance({high}).elapsed.value == high, "repeated instant at max is safe");
    StopwatchState span({low});
    span.apply(StopwatchCommand::Start, {low});
    rejects<std::overflow_error>(span, [&] { span.advance({high}); }, "full signed span rejects safely");
    check(span.apply(StopwatchCommand::Reset, {high}).snapshot.elapsed.value == 0,
          "explicit reset can discard overflowing running interval");
    span.apply(StopwatchCommand::Start, {high});
    check(span.apply(StopwatchCommand::Lap, {high}).snapshot.laps[0].total.value == 0,
          "zero lap at maximum timestamp");
    StopwatchState paused({low});
    paused.apply(StopwatchCommand::Start, {low});
    paused.apply(StopwatchCommand::Stop, {low});
    check(paused.apply(StopwatchCommand::Resume, {high}).snapshot.elapsed.value == 0,
          "full signed-span pause remains excluded");
    std::cout << checks << " stopwatch checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
