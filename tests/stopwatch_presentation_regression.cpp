#include "../src/app/stopwatch_presentation.hpp"
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
bool equal(const hamclock::StopwatchPresentationView& a,
           const hamclock::StopwatchPresentationView& b) {
    return a.phase == b.phase && a.label == b.label && a.held == b.held &&
           a.actual_elapsed.value == b.actual_elapsed.value &&
           a.displayed_elapsed.value == b.displayed_elapsed.value &&
           a.formatted == b.formatted && a.lap_count == b.lap_count;
}
template<typename Action>
void rejected(hamclock::StopwatchPresentation& presentation, Action action,
              const char* label) {
    const auto before = presentation.view();
    bool threw = false;
    try {
        action();
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, label);
    check(equal(before, presentation.view()), "invalid input preserves whole presentation");
}
}

int main() {
    using namespace hamclock;
    const struct { std::int64_t ns; const char* formatted; } examples[] = {
        {0, "00:00:00.00"}, {9999999, "00:00:00.00"}, {10000000, "00:00:00.01"},
        {19999999, "00:00:00.01"}, {999999999, "00:00:00.99"},
        {1000000000, "00:00:01.00"}, {59999999999, "00:00:59.99"},
        {60000000000, "00:01:00.00"}, {3599999999999, "00:59:59.99"},
        {3600000000000, "01:00:00.00"}, {86400000000000, "24:00:00.00"},
        {360000000000000, "100:00:00.00"},
        {std::numeric_limits<std::int64_t>::max(), "2562047:47:16.85"}
    };
    for (const auto& example : examples) {
        const auto formatted = format_stopwatch_duration({example.ns});
        check(formatted == example.formatted, "independent exact formatting fixture");
        check(formatted.size() <= kStopwatchMaximumFormattedBytes, "bounded owned formatter output");
    }
    for (auto negative : {-1LL, std::numeric_limits<long long>::min()}) {
        bool threw = false;
        try { format_stopwatch_duration({negative}); }
        catch (const std::invalid_argument&) { threw = true; }
        check(threw, "negative duration never wraps");
    }
    StopwatchPresentation display;
    check(display.view().label == StopwatchDisplayLabel::Idle &&
          display.view().formatted == "00:00:00.00", "initial idle display");
    StopwatchState clock({0});
    rejected(display, [&] { display.apply(StopwatchDisplayCommand::Hold, clock.snapshot()); },
             "cannot hold idle stopwatch");
    clock.apply(StopwatchCommand::Start, {0});
    clock.advance({1250000000});
    auto hold = display.apply(StopwatchDisplayCommand::Hold, clock.snapshot());
    check(hold.result == StopwatchDisplayResult::Applied && hold.view.held &&
          hold.view.label == StopwatchDisplayLabel::RunningHeld &&
          hold.view.formatted == "00:00:01.25", "hold latches exact current display");
    clock.advance({2250000000});
    auto still = display.refresh(clock.snapshot());
    check(still.actual_elapsed.value == 2250000000 && still.displayed_elapsed.value == 1250000000 &&
          still.formatted == "00:00:01.25", "domain advances while display held");
    clock.apply(StopwatchCommand::Lap, {2500000000});
    auto repeated = display.apply(StopwatchDisplayCommand::Hold, clock.snapshot());
    check(repeated.result == StopwatchDisplayResult::Unchanged && repeated.view.lap_count == 1 &&
          repeated.view.actual_elapsed.value == 2500000000 && repeated.view.formatted == "00:00:01.25",
          "repeated hold retains original latch but updates actual lap status");
    const auto domain_before = clock.snapshot();
    auto release = display.apply(StopwatchDisplayCommand::Release, clock.snapshot());
    check(!release.view.held && release.view.label == StopwatchDisplayLabel::Running &&
          release.view.formatted == "00:00:02.50", "named release displays current underlying elapsed");
    check(clock.snapshot().elapsed.value == domain_before.elapsed.value &&
          clock.snapshot().lap_count == domain_before.lap_count, "presentation never mutates model");
    check(display.apply(StopwatchDisplayCommand::Release, clock.snapshot()).result ==
          StopwatchDisplayResult::Unchanged, "repeated release has explicit unchanged result");
    display.apply(StopwatchDisplayCommand::Hold, clock.snapshot());
    clock.apply(StopwatchCommand::Stop, {3000000000});
    auto stopped = display.refresh(clock.snapshot());
    check(!stopped.held && stopped.label == StopwatchDisplayLabel::Stopped &&
          stopped.formatted == "00:00:03.00", "stop clears latch and shows stopped total");
    rejected(display, [&] { display.apply(StopwatchDisplayCommand::Hold, clock.snapshot()); },
             "cannot hold stopped stopwatch");
    clock.apply(StopwatchCommand::Resume, {4000000000});
    clock.advance({4500000000});
    display.apply(StopwatchDisplayCommand::Hold, clock.snapshot());
    clock.apply(StopwatchCommand::Reset, {5000000000});
    auto reset = display.refresh(clock.snapshot());
    check(reset.label == StopwatchDisplayLabel::Idle && !reset.held && reset.lap_count == 0 &&
          reset.formatted == "00:00:00.00", "domain reset clears all presentation latch state");

    StopwatchSnapshot running;
    running.phase = StopwatchPhase::Running;
    running.elapsed = {1000000000};
    display.apply(StopwatchDisplayCommand::Hold, running);
    auto later = running;
    later.elapsed = {3000000000};
    display.refresh(later);
    later.elapsed = {2000000000};
    check(display.refresh(later).displayed_elapsed.value == 1000000000 &&
          display.view().actual_elapsed.value == 2000000000,
          "held snapshot ordering checks latch rather than previous actual");
    auto lower = running;
    lower.elapsed = {500000000};
    rejected(display, [&] { display.refresh(lower); }, "held value greater than supplied actual rejected");
    check(display.apply(StopwatchDisplayCommand::Release, lower).view.formatted == "00:00:00.50",
          "explicit release can replace held value with newly supplied snapshot");
    auto copy = display.view();
    copy.formatted.assign(100, 'x');
    copy.actual_elapsed = {-1};
    check(display.view().formatted == "00:00:00.50" && display.view().actual_elapsed.value == 500000000,
          "owned returned view mutation cannot alter presentation");
    rejected(display, [&] { display.apply(static_cast<StopwatchDisplayCommand>(999), running); },
             "unknown display command rejected");
    for (int scenario = 0; scenario < 9; ++scenario) {
        auto invalid = running;
        switch (scenario) {
        case 0: invalid.phase = static_cast<StopwatchPhase>(999); break;
        case 1: invalid.elapsed = {-1}; break;
        case 2: invalid.lap_count = kStopwatchMaximumLaps + 1; break;
        case 3: invalid.lap_count = 1; invalid.laps[0] = {2, {1}, {1}}; break;
        case 4: invalid.lap_count = 1; invalid.laps[0] = {1, {-1}, {-1}}; break;
        case 5: invalid.lap_count = 1; invalid.laps[0] = {1, {1000000001}, {1000000001}}; break;
        case 6: invalid.lap_count = 1; invalid.laps[0] = {1, {1}, {0}}; break;
        case 7: invalid.laps[63].total = {1}; break;
        case 8: invalid.phase = StopwatchPhase::Idle; break;
        }
        rejected(display, [&] { display.refresh(invalid); }, "malformed public snapshot rejected");
        rejected(display, [&] { display.apply(StopwatchDisplayCommand::Release, invalid); },
                 "release does not bypass snapshot validation");
    }
    running.elapsed = {std::numeric_limits<std::int64_t>::max()};
    running.lap_count = kStopwatchMaximumLaps;
    for (std::size_t i = 0; i < running.lap_count; ++i) {
        running.laps[i] = {i + 1, {0}, {0}};
    }
    check(display.refresh(running).formatted == "2562047:47:16.85",
          "maximum duration and full zero lap list format safely");
    std::cout << checks << " stopwatch presentation checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
