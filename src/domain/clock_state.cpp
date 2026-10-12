#include "clock_state.hpp"
#include <limits>
#include <stdexcept>

namespace hamclock {
namespace {
constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
std::int64_t checked_add(std::int64_t a, std::int64_t b) {
    if ((b > 0 && a > maximum - b) || (b < 0 && a < minimum - b))
        throw std::overflow_error("Clock preview is outside signed64 UTC range");
    return a + b;
}
bool difference_fits(std::int64_t later, std::int64_t earlier) noexcept {
    // Caller has established later >= earlier. Only negative earlier can
    // make the nonnegative difference exceed signed64 maximum.
    return earlier >= 0 || later <= maximum + earlier;
}
bool valid_status(SyncStatus status) noexcept {
    return status == SyncStatus::Unknown || status == SyncStatus::Synchronized
        || status == SyncStatus::Unsynchronized;
}
SyncStatus evidence_status(const std::optional<SyncEvidence>& evidence,
                           MonotonicNanoseconds now, SyncFreshnessPolicy policy) {
    if (!evidence) return SyncStatus::Unknown;
    if (!valid_status(evidence->status))
        throw std::invalid_argument("Invalid clock synchronization evidence status");
    if (evidence->status == SyncStatus::Unknown || evidence->observed_at.value > now.value
        || !difference_fits(now.value, evidence->observed_at.value))
        return SyncStatus::Unknown;
    const auto age = now.value - evidence->observed_at.value;
    return age <= policy.maximum_age.value ? evidence->status : SyncStatus::Unknown;
}
}
ClockState::ClockState(UtcSeconds initial_utc, MonotonicNanoseconds initial_monotonic,
                       SyncFreshnessPolicy policy)
    : origin_(initial_monotonic), last_(initial_monotonic), policy_(policy),
      snapshot_{initial_utc, initial_utc, false, {0}, SyncStatus::Unknown} {
    if (policy.maximum_age.value < 0)
        throw std::invalid_argument("Clock evidence maximum age must be nonnegative");
}
const ClockSnapshot& ClockState::snapshot() const noexcept { return snapshot_; }
ClockSnapshot ClockState::advance(UtcSeconds actual_utc, MonotonicNanoseconds monotonic,
                                  std::optional<SyncEvidence> evidence) {
    if (monotonic.value < last_.value)
        throw std::invalid_argument("Injected monotonic clock moved backwards");
    if (!difference_fits(monotonic.value, origin_.value))
        throw std::overflow_error("Clock elapsed duration exceeds signed64 nanoseconds");
    const auto display = checked_add(actual_utc.value, offset_.value);
    const auto status = evidence_status(evidence, monotonic, policy_);
    const ClockSnapshot next{actual_utc, {display}, snapshot_.preview,
                             {monotonic.value - origin_.value}, status};
    // Every potentially failing computation precedes the state commit.
    snapshot_ = next;
    last_ = monotonic;
    return next;
}
void ClockState::set_preview_offset(UtcOffsetSeconds offset) {
    const auto display = checked_add(snapshot_.actual_utc.value, offset.value);
    offset_ = offset;
    snapshot_.display_utc = {display};
    snapshot_.preview = true;
}
void ClockState::reset_preview() noexcept {
    offset_ = {0};
    snapshot_.display_utc = snapshot_.actual_utc;
    snapshot_.preview = false;
}
} // namespace hamclock
