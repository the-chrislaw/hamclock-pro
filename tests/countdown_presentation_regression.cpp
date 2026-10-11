#include "../src/app/countdown_presentation.hpp"
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
        a.remaining.value == b.remaining.value && a.run_id == b.run_id && a.expiry_pending == b.expiry_pending;
}
bool same(const hamclock::CountdownPresentationView& a, const hamclock::CountdownPresentationView& b) {
    return a.phase == b.phase && a.configured.value == b.configured.value &&
        a.remaining.value == b.remaining.value && a.run_id == b.run_id && a.expiry_pending == b.expiry_pending &&
        a.dismiss_available == b.dismiss_available && a.configured_text == b.configured_text &&
        a.remaining_text == b.remaining_text && a.status == b.status;
}
} // namespace

int main() {
    using namespace hamclock;
    const struct { std::int64_t nanoseconds; const char* text; } fixtures[] = {
        {0, "00:00:00"}, {1, "00:00:01"}, {999999999, "00:00:01"},
        {1000000000, "00:00:01"}, {1000000001, "00:00:02"},
        {59999999999, "00:01:00"}, {60000000000, "00:01:00"},
        {60000000001, "00:01:01"}, {3599999999999, "01:00:00"},
        {3600000000000, "01:00:00"}, {3600000000001, "01:00:01"},
        {86399999999999, "24:00:00"}, {86400000000000, "24:00:00"},
        {86400000000001, "24:00:01"}, {604799000000000, "167:59:59"},
        {604799000000001, "168:00:00"}, {604800000000000, "168:00:00"}
    };
    for (const auto& fixture : fixtures) {
        const auto text = format_countdown_duration({fixture.nanoseconds});
        check(text == fixture.text, "independent ceil formatting fixture");
        check(text.size() <= kCountdownMaximumFormattedBytes, "owned output within nine bytes");
    }
    for (auto duration : {std::numeric_limits<std::int64_t>::min(), std::int64_t{-1},
                          kCountdownMaximumNanoseconds + 1, std::numeric_limits<std::int64_t>::max()}) {
        bool threw = false;
        try { format_countdown_duration({duration}); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "formatter rejects unrepresentable product duration without wrap");
    }
    CountdownState clock({kCountdownMinimumNanoseconds + 1}, {0});
    auto ready = make_countdown_presentation(clock.snapshot());
    check(ready.status == "Ready" && ready.configured_text == "00:01:01" && ready.run_id == 0 &&
          !ready.dismiss_available && ready.configured.value == kCountdownMinimumNanoseconds + 1,
          "idle display retains raw configured duration despite replacement ceil choice");
    clock.apply(CountdownCommand::Start, {0});
    clock.advance({kCountdownMinimumNanoseconds});
    auto running = make_countdown_presentation(clock.snapshot());
    check(running.status == "Running" && running.remaining.value == 1 && running.remaining_text == "00:00:01",
          "positive running nanosecond never displays premature zero");
    clock.apply(CountdownCommand::Stop, {kCountdownMinimumNanoseconds});
    auto paused = make_countdown_presentation(clock.snapshot());
    check(paused.status == "Paused" && paused.remaining_text == "00:00:01" && !paused.dismiss_available,
          "paused positive fraction also rounds up truthfully");
    clock.apply(CountdownCommand::Resume, {kCountdownMinimumNanoseconds});
    clock.advance({kCountdownMinimumNanoseconds + 1});
    auto expired = make_countdown_presentation(clock.snapshot());
    check(expired.status == "Time elapsed - alert pending" && expired.dismiss_available &&
          expired.expiry_pending && expired.remaining_text == "00:00:00" && expired.run_id == 1,
          "expiry conveys zero and pending affordance with current run identity");
    clock.acknowledge(1, {kCountdownMinimumNanoseconds + 1});
    auto acknowledged = make_countdown_presentation(clock.snapshot());
    check(acknowledged.phase == CountdownPhase::Expired && acknowledged.status == "Time elapsed" &&
          acknowledged.remaining_text == "00:00:00" && !acknowledged.dismiss_available,
          "acknowledgment remains expired zero without silently activating timer");
    clock.apply(CountdownCommand::Reset, {kCountdownMinimumNanoseconds + 1});
    check(make_countdown_presentation(clock.snapshot()).run_id == 1, "Idle may retain previous run identity after Reset");
    auto copy = expired;
    copy.remaining_text.assign(100, 'x');
    copy.status = "wrong";
    check(expired.remaining_text == "00:00:00" && expired.status == "Time elapsed - alert pending",
          "returned owned strings cannot alter earlier view");

    const CountdownSnapshot valid{CountdownPhase::Running, {kCountdownMinimumNanoseconds}, {1}, 1, false};
    for (int scenario = 0; scenario < 15; ++scenario) {
        auto invalid = valid;
        switch (scenario) {
        case 0: invalid.phase = static_cast<CountdownPhase>(999); break;
        case 1: invalid.configured.value = kCountdownMinimumNanoseconds - 1; break;
        case 2: invalid.configured.value = kCountdownMaximumNanoseconds + 1; break;
        case 3: invalid.remaining.value = -1; break;
        case 4: invalid.remaining.value = invalid.configured.value + 1; break;
        case 5: invalid.run_id = 0; break;
        case 6: invalid.remaining.value = 0; break;
        case 7: invalid.expiry_pending = true; break;
        case 8: invalid.phase = CountdownPhase::Paused; invalid.run_id = 0; break;
        case 9: invalid.phase = CountdownPhase::Paused; invalid.remaining.value = 0; break;
        case 10: invalid.phase = CountdownPhase::Paused; invalid.expiry_pending = true; break;
        case 11: invalid.phase = CountdownPhase::Idle; break;
        case 12: invalid.phase = CountdownPhase::Idle; invalid.remaining = invalid.configured; invalid.expiry_pending = true; break;
        case 13: invalid.phase = CountdownPhase::Expired; break;
        case 14: invalid.phase = CountdownPhase::Expired; invalid.remaining.value = 0; invalid.run_id = 0; break;
        }
        const auto before = invalid;
        const auto prior_view = acknowledged;
        bool threw = false;
        try { acknowledged = make_countdown_presentation(invalid); }
        catch (const std::invalid_argument&) { threw = true; }
        check(threw, "fabricated snapshot invariant rejected");
        check(same(invalid, before) && same(acknowledged, prior_view), "failure preserves immutable input and caller prior view");
    }
    auto max = valid;
    max.configured = max.remaining = {kCountdownMaximumNanoseconds};
    max.run_id = std::numeric_limits<std::uint64_t>::max();
    check(make_countdown_presentation(max).remaining_text == "168:00:00", "maximum config and identity render bounded owned values");
    check(same(valid, CountdownSnapshot{CountdownPhase::Running, {kCountdownMinimumNanoseconds}, {1}, 1, false}),
          "successful conversion leaves source unchanged");
    std::cout << checks << " countdown presentation checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
