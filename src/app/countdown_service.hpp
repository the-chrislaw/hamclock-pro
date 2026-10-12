#pragma once

#include "countdown_presentation.hpp"
#include <memory>
#include <optional>

namespace hamclock {
namespace detail { struct CountdownServiceIdentity; }
class CountdownService;

// Opaque authority, issued only for a pending event. Copying preserves the
// captured event, never follows current state. Weak lifetime ownership prevents
// old-object address reuse from validating a token after service destruction.
class CountdownDismissToken {
public:
    CountdownDismissToken(const CountdownDismissToken&) = default;
    CountdownDismissToken(CountdownDismissToken&&) noexcept = default;
    CountdownDismissToken& operator=(const CountdownDismissToken&) = default;
    CountdownDismissToken& operator=(CountdownDismissToken&&) noexcept = default;
private:
    friend class CountdownService;
    CountdownDismissToken(std::weak_ptr<const detail::CountdownServiceIdentity>,
                          std::uint64_t epoch, std::uint64_t run);
    std::weak_ptr<const detail::CountdownServiceIdentity> identity_;
    std::uint64_t epoch_;
    std::uint64_t run_;
};
struct CountdownServiceSnapshot {
    std::uint64_t configuration_epoch;
    CountdownSnapshot model;
    CountdownPresentationView view;
    std::optional<CountdownDismissToken> dismiss_token;
};
struct CountdownServiceTransition {
    CountdownCommandResult result;
    CountdownServiceSnapshot snapshot;
};
// Pure checked epoch increment, used before every successful reconfiguration.
// Full uint64 range; maximum throws overflow_error before reuse/wrap.
std::uint64_t next_countdown_configuration_epoch(std::uint64_t previous);

// Original single-owner pure session service, not thread safe. All callers must
// serialize operations. Owns exactly one model and presentation plus an opaque
// lifetime identity; no global IDs, actual clock reads, Qt, callbacks, IO or
// persistence. Injected timestamps only, including constructor instant.
// Copy/move deleted so service identity cannot alias a second active owner.
// Initial epoch1/run0. snapshot returns completely owned model/view/token.
// Pending-only token binds instance+epoch+run; acknowledge rejects expired or
// foreign lifetime, prior epoch/run, or no longer pending event invalid_argument.
// Validation precedes any time advance; an old banner cannot clear a new event.
// Tick/apply delegate domain semantics, then prepare presentation and owned
// returned snapshot BEFORE no-throw commits. Any exception preserves entire
// model, view, last instant, epoch and token authority. No partial expiry/update.
// reconfigure validates new inclusive1min..7day duration and time ordering,
// allocates next epoch, prepares new Idle/full/run0 model and view, then commits.
// Same duration ALSO replaces/cancels; old pending/running state is explicitly
// discarded without advancing it. Epoch distinguishes restarted run numbers.
// Configuration/instance tokens are session-only, not reboot-safe identities.
class CountdownService {
public:
    CountdownService(ElapsedNanoseconds configured, MonotonicNanoseconds initial);
    ~CountdownService();
    CountdownService(const CountdownService&) = delete;
    CountdownService& operator=(const CountdownService&) = delete;
    CountdownService(CountdownService&&) = delete;
    CountdownService& operator=(CountdownService&&) = delete;
    CountdownServiceSnapshot snapshot() const;
    MonotonicNanoseconds last_observed() const noexcept;
    CountdownServiceSnapshot advance(MonotonicNanoseconds);
    CountdownServiceTransition apply(CountdownCommand, MonotonicNanoseconds);
    CountdownServiceTransition acknowledge(const CountdownDismissToken&, MonotonicNanoseconds);
    CountdownServiceSnapshot reconfigure(ElapsedNanoseconds, MonotonicNanoseconds);
private:
    CountdownServiceSnapshot prepare_snapshot(const CountdownState&,
                                             const CountdownPresentationView&,
                                             std::uint64_t epoch) const;
    CountdownState model_;
    CountdownPresentationView view_;
    std::shared_ptr<const detail::CountdownServiceIdentity> identity_;
    std::uint64_t epoch_ = 1;
};
} // namespace hamclock
