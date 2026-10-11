#include "countdown_service.hpp"
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hamclock {
namespace detail { struct CountdownServiceIdentity {}; }
namespace {
static_assert(std::is_nothrow_swappable_v<CountdownState>);
static_assert(std::is_nothrow_swappable_v<CountdownPresentationView>);
static_assert(std::is_nothrow_move_constructible_v<CountdownServiceSnapshot>);
static_assert(std::is_nothrow_move_constructible_v<CountdownServiceTransition>);
void validate_order(MonotonicNanoseconds now, MonotonicNanoseconds last) {
    if (now.value < last.value) throw std::invalid_argument("Reversed countdown service monotonic time");
}
} // namespace
CountdownDismissToken::CountdownDismissToken(
    std::weak_ptr<const detail::CountdownServiceIdentity> identity,
    std::uint64_t epoch, std::uint64_t run)
    : identity_(std::move(identity)), epoch_(epoch), run_(run) {}

std::uint64_t next_countdown_configuration_epoch(std::uint64_t previous) {
    if (previous == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Countdown configuration epochs exhausted");
    }
    return previous + 1;
}

CountdownService::CountdownService(ElapsedNanoseconds configured, MonotonicNanoseconds initial)
    : model_(configured, initial), view_(make_countdown_presentation(model_.snapshot())),
      identity_(std::make_shared<const detail::CountdownServiceIdentity>()) {}
CountdownService::~CountdownService() = default;

CountdownServiceSnapshot CountdownService::prepare_snapshot(
    const CountdownState& model, const CountdownPresentationView& view, std::uint64_t epoch) const {
    CountdownServiceSnapshot output{epoch, model.snapshot(), view, std::nullopt};
    if (output.model.expiry_pending) {
        output.dismiss_token = CountdownDismissToken{identity_, epoch, output.model.run_id};
    }
    return output;
}
CountdownServiceSnapshot CountdownService::snapshot() const {
    return prepare_snapshot(model_, view_, epoch_);
}
MonotonicNanoseconds CountdownService::last_observed() const noexcept { return model_.last_observed(); }

CountdownServiceSnapshot CountdownService::advance(MonotonicNanoseconds now) {
    auto model = model_;
    model.advance(now);
    auto view = make_countdown_presentation(model.snapshot());
    auto output = prepare_snapshot(model, view, epoch_);
    using std::swap;
    swap(model_, model);
    swap(view_, view);
    return output;
}
CountdownServiceTransition CountdownService::apply(CountdownCommand command, MonotonicNanoseconds now) {
    auto model = model_;
    const auto result = model.apply(command, now).result;
    auto view = make_countdown_presentation(model.snapshot());
    CountdownServiceTransition output{result, prepare_snapshot(model, view, epoch_)};
    using std::swap;
    swap(model_, model);
    swap(view_, view);
    return output;
}
CountdownServiceTransition CountdownService::acknowledge(
    const CountdownDismissToken& token, MonotonicNanoseconds now) {
    const auto identity = token.identity_.lock();
    if (!identity || identity != identity_ || token.epoch_ != epoch_ ||
        token.run_ != model_.snapshot().run_id || !model_.snapshot().expiry_pending) {
        throw std::invalid_argument("Countdown dismissal token is no longer valid for this event");
    }
    auto model = model_;
    const auto result = model.acknowledge(token.run_, now).result;
    auto view = make_countdown_presentation(model.snapshot());
    CountdownServiceTransition output{result, prepare_snapshot(model, view, epoch_)};
    using std::swap;
    swap(model_, model);
    swap(view_, view);
    return output;
}
CountdownServiceSnapshot CountdownService::reconfigure(
    ElapsedNanoseconds configured, MonotonicNanoseconds now) {
    validate_order(now, model_.last_observed());
    CountdownState model(configured, now);
    const auto epoch = next_countdown_configuration_epoch(epoch_);
    auto view = make_countdown_presentation(model.snapshot());
    auto output = prepare_snapshot(model, view, epoch);
    using std::swap;
    swap(model_, model);
    swap(view_, view);
    epoch_ = epoch;
    return output;
}
} // namespace hamclock
