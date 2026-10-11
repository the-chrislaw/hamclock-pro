#include "../src/app/countdown_service.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace fault {
std::size_t attempts = 0;
std::size_t fail_at = std::numeric_limits<std::size_t>::max();
}
void* operator new(std::size_t bytes) {
    if (fault::attempts++ == fault::fail_at) throw std::bad_alloc();
    if (auto* memory = std::malloc(bytes == 0 ? 1 : bytes)) return memory;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

namespace {
int checks = 0, failures = 0;
void check(bool okay, const char* label) {
    ++checks;
    if (!okay) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool same(const hamclock::CountdownServiceSnapshot& a, const hamclock::CountdownServiceSnapshot& b) {
    const auto& x = a.model;
    const auto& y = b.model;
    const auto& p = a.view;
    const auto& q = b.view;
    return a.configuration_epoch == b.configuration_epoch && x.phase == y.phase &&
        x.configured.value == y.configured.value && x.remaining.value == y.remaining.value &&
        x.run_id == y.run_id && x.expiry_pending == y.expiry_pending &&
        p.phase == q.phase && p.configured.value == q.configured.value &&
        p.remaining.value == q.remaining.value && p.run_id == q.run_id && p.expiry_pending == q.expiry_pending &&
        p.dismiss_available == q.dismiss_available && p.status == q.status &&
        p.configured_text == q.configured_text && p.remaining_text == q.remaining_text &&
        a.dismiss_token.has_value() == b.dismiss_token.has_value();
}
template<typename Exception, typename Action>
void rejected(hamclock::CountdownService& service, Action action, const char* label) {
    const auto before = service.snapshot();
    const auto last = service.last_observed();
    bool threw = false;
    try { action(); } catch (const Exception&) { threw = true; }
    check(threw, label);
    check(same(before, service.snapshot()) && last.value == service.last_observed().value,
          "failed operation preserves model view epoch token presence and last instant");
}
} // namespace

int main() {
    using namespace hamclock;
    const auto duration = kCountdownMinimumNanoseconds;
    CountdownService service({duration}, {0});
    check(service.snapshot().configuration_epoch == 1 && !service.snapshot().dismiss_token &&
          service.snapshot().view.status == "Ready", "service starts idle epoch1 with no authority token");
    auto start = service.apply(CountdownCommand::Start, {0});
    check(start.snapshot.model.run_id == 1 && start.snapshot.view.status == "Running" &&
          !start.snapshot.dismiss_token, "Start updates consistent model and owned view without dismissal authority");
    auto expired = service.advance({duration});
    check(expired.model.expiry_pending && expired.view.dismiss_available && expired.dismiss_token,
          "pending expiry creates captured service epoch run dismissal token");
    auto token = *expired.dismiss_token;
    rejected<std::invalid_argument>(service, [&] { service.acknowledge(token, {duration - 1}); },
                                    "valid token still rejects reversed monotonic instant");
    auto acknowledged = service.acknowledge(token, {duration});
    check(acknowledged.result == CountdownCommandResult::Applied &&
          acknowledged.snapshot.model.phase == CountdownPhase::Expired && !acknowledged.snapshot.dismiss_token &&
          acknowledged.snapshot.view.status == "Time elapsed", "matching token clears event across consistent owned snapshot");
    rejected<std::invalid_argument>(service, [&] { service.acknowledge(token, {duration + 1}); },
                                    "repeated consumed token rejected without clock advance");
    service.apply(CountdownCommand::Start, {duration});
    service.advance({duration * 2});
    rejected<std::invalid_argument>(service, [&] { service.acknowledge(token, {duration * 2 + 1}); },
                                    "prior run token cannot clear new pending run");
    auto new_token = *service.snapshot().dismiss_token;
    CountdownService foreign({duration}, {0});
    foreign.apply(CountdownCommand::Start, {0});
    auto foreign_expired = foreign.advance({duration});
    rejected<std::invalid_argument>(foreign, [&] { foreign.acknowledge(new_token, {duration}); },
                                    "active foreign service same run identity rejected");
    rejected<std::invalid_argument>(service, [&] { service.acknowledge(*foreign_expired.dismiss_token, {duration * 2}); },
                                    "foreign token cannot clear original pending event");
    auto moved_token = std::move(new_token);
    rejected<std::invalid_argument>(service, [&] { service.acknowledge(new_token, {duration * 2}); },
                                    "moved from weak identity token rejected");
    check(service.acknowledge(moved_token, {duration * 2}).result == CountdownCommandResult::Applied,
          "moved valid token preserves authority for captured event");

    service.apply(CountdownCommand::Start, {duration * 2});
    auto previous = service.advance({duration * 3});
    auto prior_epoch_token = *previous.dismiss_token;
    rejected<std::invalid_argument>(service, [&] { service.reconfigure({duration - 1}, {duration * 3 + 1}); },
                                    "invalid new duration preserves pending and old token authority");
    rejected<std::invalid_argument>(service, [&] { service.reconfigure({duration}, {0}); },
                                    "reconfiguration rejects reversed time");
    auto replaced = service.reconfigure({duration}, {duration * 3});
    check(replaced.configuration_epoch == 2 && replaced.model.run_id == 0 &&
          replaced.model.phase == CountdownPhase::Idle && !replaced.dismiss_token &&
          replaced.model.remaining.value == duration, "same duration explicitly replaces cancels and advances epoch");
    service.apply(CountdownCommand::Start, {duration * 3});
    service.advance({duration * 4});
    rejected<std::invalid_argument>(service, [&] { service.acknowledge(prior_epoch_token, {duration * 4}); },
                                    "prior configuration token rejected despite restarted run number reuse");
    auto current = *service.snapshot().dismiss_token;
    check(service.acknowledge(current, {duration * 4}).result == CountdownCommandResult::Applied,
          "current epoch token remains usable after stale token rejection");
    rejected<std::invalid_argument>(service, [&] { service.apply(static_cast<CountdownCommand>(999), {duration * 4 + 1}); },
                                    "unknown command preserves entire shared service");

    std::optional<CountdownDismissToken> dead_token;
    {
        CountdownService dead({duration}, {0});
        dead.apply(CountdownCommand::Start, {0});
        dead_token = dead.advance({duration}).dismiss_token;
    }
    rejected<std::invalid_argument>(foreign, [&] { foreign.acknowledge(*dead_token, {duration + 1}); },
                                    "destroyed lifetime token cannot acquire a new active service");
    auto owned = foreign.snapshot();
    owned.view.status = "mutated";
    owned.model.remaining.value = 99;
    owned.dismiss_token.reset();
    check(foreign.snapshot().view.status == "Time elapsed - alert pending" && foreign.snapshot().dismiss_token,
          "returned snapshot mutation never alters caller service or token availability");

    check(next_countdown_configuration_epoch(0) == 1 &&
          next_countdown_configuration_epoch(std::numeric_limits<std::uint64_t>::max() - 1) ==
          std::numeric_limits<std::uint64_t>::max(), "epoch helper full uint64 boundaries");
    bool exhausted = false;
    try { next_countdown_configuration_epoch(std::numeric_limits<std::uint64_t>::max()); }
    catch (const std::overflow_error&) { exhausted = true; }
    check(exhausted, "epoch helper rejects wrap without fabricated service seed");
    int allocation_failures = 0;
    int allocation_successes = 0;
    for (std::size_t fail_index = 0; fail_index < 10; ++fail_index) {
        CountdownService candidate({duration}, {0});
        candidate.apply(CountdownCommand::Start, {0});
        const auto before = candidate.snapshot();
        fault::attempts = 0;
        fault::fail_at = fail_index;
        bool failed = false;
        try { candidate.advance({duration}); } catch (const std::bad_alloc&) { failed = true; }
        fault::fail_at = std::numeric_limits<std::size_t>::max();
        if (failed) {
            ++allocation_failures;
            check(same(before, candidate.snapshot()) && candidate.last_observed().value == 0,
                  "allocation fault in expiry formatting or owned output preserves all service state");
            auto recovery = candidate.advance({duration});
            check(recovery.dismiss_token.has_value(), "successful retry creates genuine pending authority after allocation rejection");
        } else {
            ++allocation_successes;
            check(candidate.snapshot().dismiss_token.has_value(), "injection beyond actual allocations permits full expiry commit");
        }
    }
    check(allocation_failures >= 2 && allocation_successes > 0,
          "injected format and return copy allocation failures exercised on installed string ABI");
    // Existing pending snapshot copy can allocate, but observation must not
    // consume or invalidate dismissal authority when that copy fails.
    const auto pending_before = foreign.snapshot();
    fault::attempts = 0;
    fault::fail_at = 0;
    bool observation_failed = false;
    try { foreign.snapshot(); } catch (const std::bad_alloc&) { observation_failed = true; }
    fault::fail_at = std::numeric_limits<std::size_t>::max();
    check(observation_failed && same(pending_before, foreign.snapshot()), "failed owned observation preserves pending service state");
    check(foreign.acknowledge(*pending_before.dismiss_token, {duration}).result == CountdownCommandResult::Applied,
          "preexisting token remains authoritative after observation allocation failure");
    CountdownService boundaries({kCountdownMaximumNanoseconds}, {std::numeric_limits<std::int64_t>::min()});
    boundaries.apply(CountdownCommand::Start, {std::numeric_limits<std::int64_t>::min()});
    boundaries.apply(CountdownCommand::Stop, {std::numeric_limits<std::int64_t>::min() + 1});
    auto frozen = boundaries.advance({std::numeric_limits<std::int64_t>::max() - 1});
    check(frozen.model.phase == CountdownPhase::Paused && frozen.model.remaining.value == kCountdownMaximumNanoseconds - 1 &&
          frozen.view.status == "Paused" && !frozen.dismiss_token, "paused full signed span preserves both model and truthful view");
    boundaries.apply(CountdownCommand::Resume, {std::numeric_limits<std::int64_t>::max() - 1});
    check(boundaries.advance({std::numeric_limits<std::int64_t>::max()}).model.remaining.value == kCountdownMaximumNanoseconds - 2,
          "service resume excludes long paused time");
    auto cancelled = boundaries.reconfigure({duration}, {std::numeric_limits<std::int64_t>::max()});
    check(cancelled.configuration_epoch == 2 && cancelled.view.status == "Ready" && !cancelled.dismiss_token &&
          cancelled.model.configured.value == duration, "active reconfiguration cancels without issuing discarded expiry");
    CountdownService huge_jump({duration}, {std::numeric_limits<std::int64_t>::min()});
    huge_jump.apply(CountdownCommand::Start, {std::numeric_limits<std::int64_t>::min()});
    auto saturated = huge_jump.advance({std::numeric_limits<std::int64_t>::max()});
    check(saturated.model.remaining.value == 0 && saturated.view.remaining_text == "00:00:00" && saturated.dismiss_token,
          "full signed span expiry atomically creates consistent owned token and zero view");
    rejected<std::invalid_argument>(huge_jump, [&] { huge_jump.advance({0}); }, "reverse after extreme expiry preserves token and last instant");
    check(huge_jump.acknowledge(*saturated.dismiss_token, {std::numeric_limits<std::int64_t>::max()}).result == CountdownCommandResult::Applied,
          "extreme pending token remains valid after reversed tick rejection");
    std::cout << checks << " countdown service checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
