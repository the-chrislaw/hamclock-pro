#include "../src/domain/countdown_state.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
int checks = 0, failures = 0;
void check(bool okay, const char* label) {
    ++checks;
    if (!okay) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool same(const hamclock::CountdownSnapshot& a, const hamclock::CountdownSnapshot& b) {
    return a.phase == b.phase && a.configured.value == b.configured.value &&
        a.remaining.value == b.remaining.value && a.run_id == b.run_id &&
        a.expiry_pending == b.expiry_pending;
}
template<typename Exception, typename Action>
void rejected(hamclock::CountdownState& clock, Action action, const char* label) {
    const auto snapshot = clock.snapshot();
    const auto last = clock.last_observed();
    bool threw = false;
    try { action(); } catch (const Exception&) { threw = true; }
    check(threw, label);
    check(same(snapshot, clock.snapshot()) && last.value == clock.last_observed().value,
          "rejection preserves entire snapshot and observed monotonic instant");
}
} // namespace

int main() {
    using namespace hamclock;
    constexpr auto minimum = kCountdownMinimumNanoseconds;
    const auto lowest = std::numeric_limits<std::int64_t>::min();
    const auto highest = std::numeric_limits<std::int64_t>::max();
    for (auto duration : {lowest, std::int64_t{-1}, std::int64_t{0}, minimum - 1,
                          kCountdownMaximumNanoseconds + 1, highest}) {
        bool threw = false;
        try { CountdownState invalid({duration}, {0}); }
        catch (const std::invalid_argument&) { threw = true; }
        check(threw, "duration excludes zero negative below minimum and above maximum");
    }
    check(next_countdown_run_id(0) == 1 &&
          next_countdown_run_id(std::numeric_limits<std::uint64_t>::max() - 1) ==
          std::numeric_limits<std::uint64_t>::max(), "checked full range identity helper boundaries");
    bool exhausted = false;
    try { next_countdown_run_id(std::numeric_limits<std::uint64_t>::max()); }
    catch (const std::overflow_error&) { exhausted = true; }
    check(exhausted, "identity helper cannot wrap exhausted counter");
    CountdownState countdown({minimum}, {-10});
    check(countdown.snapshot().phase == CountdownPhase::Idle && countdown.snapshot().run_id == 0 &&
          countdown.snapshot().remaining.value == minimum && !countdown.snapshot().expiry_pending, "initial configured idle state");
    check(countdown.advance({0}).remaining.value == minimum, "idle time never counts");
    rejected<std::logic_error>(countdown, [&] { countdown.apply(CountdownCommand::Resume, {1}); }, "idle resume rejected");
    rejected<std::invalid_argument>(countdown, [&] { countdown.acknowledge(0, {1}); }, "zero identity rejected");
    auto first = countdown.apply(CountdownCommand::Start, {0});
    check(first.result == CountdownCommandResult::Applied && first.snapshot.run_id == 1, "first Start creates instance run identity");
    check(countdown.advance({0}).remaining.value == minimum, "repeat instant does not decrement");
    auto before = countdown.advance({minimum - 1});
    check(before.phase == CountdownPhase::Running && before.remaining.value == 1 && !before.expiry_pending, "one nanosecond before expiry still running");
    auto expired = countdown.advance({minimum});
    check(expired.phase == CountdownPhase::Expired && expired.remaining.value == 0 && expired.expiry_pending, "at boundary creates one pending expiry");
    check(countdown.advance({highest}).expiry_pending && countdown.snapshot().remaining.value == 0, "expired later tick stays saturated pending");
    check(countdown.acknowledge(1, {highest}).result == CountdownCommandResult::Applied &&
          !countdown.snapshot().expiry_pending, "matching acknowledgement clears pending");
    check(countdown.advance({highest}).phase == CountdownPhase::Expired && !countdown.snapshot().expiry_pending,
          "ticks cannot rearm acknowledged expiry");
    check(countdown.acknowledge(1, {highest}).result == CountdownCommandResult::Unchanged, "repeat acknowledgement unchanged");
    rejected<std::logic_error>(countdown, [&] { countdown.apply(CountdownCommand::Resume, {highest}); }, "expired resume rejected");
    countdown.apply(CountdownCommand::Reset, {highest});
    check(countdown.snapshot().phase == CountdownPhase::Idle && countdown.snapshot().run_id == 1 &&
          countdown.snapshot().remaining.value == minimum, "Reset cancels and preserves generation");
    countdown.apply(CountdownCommand::Start, {highest});
    check(countdown.snapshot().run_id == 2 && !countdown.snapshot().expiry_pending, "restart allocates different identity");
    rejected<std::invalid_argument>(countdown, [&] { countdown.acknowledge(1, {highest}); }, "stale previous run acknowledgement rejected");

    CountdownState paused({minimum}, {lowest});
    paused.apply(CountdownCommand::Start, {lowest});
    paused.apply(CountdownCommand::Stop, {lowest + 1000});
    check(paused.snapshot().phase == CountdownPhase::Paused && paused.snapshot().remaining.value == minimum - 1000,
          "Stop first counts running interval then pauses");
    paused.advance({highest - 1000});
    check(paused.snapshot().remaining.value == minimum - 1000, "huge paused span excluded");
    check(paused.apply(CountdownCommand::Stop, {highest - 1000}).result == CountdownCommandResult::Unchanged,
          "repeated paused Stop remains frozen");
    paused.apply(CountdownCommand::Resume, {highest - 1000});
    check(paused.advance({highest}).remaining.value == minimum - 2000, "resume counts only new running interval");

    CountdownState jump({kCountdownMaximumNanoseconds}, {lowest});
    jump.apply(CountdownCommand::Start, {lowest});
    check(jump.advance({highest}).remaining.value == 0 && jump.snapshot().expiry_pending,
          "full signed64 monotonic span saturates without arithmetic overflow");
    rejected<std::invalid_argument>(jump, [&] { jump.advance({lowest}); }, "reversed signed64 span rejected");

    CountdownState commands({minimum}, {0});
    check(commands.apply(CountdownCommand::Stop, {1}).result == CountdownCommandResult::Unchanged,
          "idle Stop unchanged but observes time");
    commands.apply(CountdownCommand::Start, {1});
    commands.advance({1001});
    commands.apply(CountdownCommand::Start, {1001});
    check(commands.snapshot().run_id == 2 && commands.snapshot().remaining.value == minimum,
          "Start while running explicitly restarts full duration");
    commands.apply(CountdownCommand::Stop, {1001 + minimum});
    check(commands.snapshot().phase == CountdownPhase::Expired && commands.snapshot().expiry_pending,
          "Stop at expiry cannot pause zero remaining");
    const auto old_id = commands.snapshot().run_id;
    commands.apply(CountdownCommand::Start, {1001 + minimum});
    commands.advance({1001 + minimum * 2});
    rejected<std::invalid_argument>(commands, [&] { commands.acknowledge(old_id, {1002 + minimum * 2}); },
                                    "stale ack cannot clear new expired event or advance time");
    check(commands.snapshot().expiry_pending, "new run remains pending after stale acknowledgment");

    for (auto command : {CountdownCommand::Start, CountdownCommand::Stop, CountdownCommand::Resume, CountdownCommand::Reset}) {
        rejected<std::invalid_argument>(commands, [&] { commands.apply(command, {0}); }, "all commands reject reversed time atomically");
    }
    rejected<std::invalid_argument>(commands, [&] { commands.apply(static_cast<CountdownCommand>(999), {highest}); }, "unknown command atomic rejection");
    rejected<std::invalid_argument>(commands, [&] { commands.acknowledge(commands.snapshot().run_id, {0}); }, "matching ack still rejects reversed clock");

    CountdownState direct_ack({minimum}, {0});
    direct_ack.apply(CountdownCommand::Start, {0});
    check(direct_ack.acknowledge(1, {minimum - 1}).result == CountdownCommandResult::Unchanged &&
          direct_ack.snapshot().remaining.value == 1, "early matching ack advances without inventing expiry");
    check(direct_ack.acknowledge(1, {minimum}).result == CountdownCommandResult::Applied &&
          direct_ack.snapshot().phase == CountdownPhase::Expired && !direct_ack.snapshot().expiry_pending,
          "at expiry matching ack observes and clears event in one explicit operation");
    check(!direct_ack.advance({minimum + 1}).expiry_pending, "direct boundary ack never rearms");
    direct_ack.apply(CountdownCommand::Reset, {minimum + 1});
    check(direct_ack.apply(CountdownCommand::Reset, {minimum + 1}).result == CountdownCommandResult::Unchanged,
          "repeated Reset idle no change");
    CountdownState resume_tick({minimum}, {-minimum});
    resume_tick.apply(CountdownCommand::Start, {-minimum});
    const auto repeated_resume = resume_tick.apply(CountdownCommand::Resume, {-1});
    check(repeated_resume.result == CountdownCommandResult::Unchanged &&
          repeated_resume.snapshot.remaining.value == 1, "Running Resume unchanged still advances to remaining one");
    check(resume_tick.apply(CountdownCommand::Resume, {0}).snapshot.phase == CountdownPhase::Expired,
          "Running Resume at deadline expires instead of restarting");
    CountdownState exact_max({kCountdownMaximumNanoseconds}, {-kCountdownMaximumNanoseconds});
    exact_max.apply(CountdownCommand::Start, {-kCountdownMaximumNanoseconds});
    check(exact_max.advance({-1}).remaining.value == 1 && exact_max.advance({0}).remaining.value == 0,
          "inclusive seven-day maximum exact deadline handles negative initial epoch");
    exact_max.apply(CountdownCommand::Start, {1});
    exact_max.apply(CountdownCommand::Reset, {highest});
    check(exact_max.snapshot().remaining.value == kCountdownMaximumNanoseconds &&
          exact_max.snapshot().phase == CountdownPhase::Idle && !exact_max.snapshot().expiry_pending,
          "Reset discards huge active interval without inventing pending event");
    std::cout << checks << " countdown checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
