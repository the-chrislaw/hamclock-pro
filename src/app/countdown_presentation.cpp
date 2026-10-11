#include "countdown_presentation.hpp"
#include <stdexcept>

namespace hamclock {
namespace {
std::string two_digits(std::int64_t value) {
    std::string text(2, '0');
    text[0] = static_cast<char>('0' + value / 10);
    text[1] = static_cast<char>('0' + value % 10);
    return text;
}
std::string status_text(const CountdownSnapshot& snapshot) {
    switch (snapshot.phase) {
    case CountdownPhase::Idle: return "Ready";
    case CountdownPhase::Running: return "Running";
    case CountdownPhase::Paused: return "Paused";
    case CountdownPhase::Expired:
        return snapshot.expiry_pending ? "Time elapsed - alert pending" : "Time elapsed";
    }
    throw std::invalid_argument("Unknown countdown phase");
}
} // namespace

std::string format_countdown_duration(ElapsedNanoseconds duration) {
    if (duration.value < 0 || duration.value > kCountdownMaximumNanoseconds) {
        throw std::invalid_argument("Countdown display duration out of range");
    }
    // Avoid adding a billion to the numerator. Positive fraction gets one
    // extra second; all intermediate integers stay within the seven-day cap.
    const auto seconds = duration.value / 1000000000 + (duration.value % 1000000000 != 0 ? 1 : 0);
    const auto minutes = seconds / 60;
    auto hours = std::to_string(minutes / 60);
    if (hours.size() < 2) hours.insert(0, 1, '0');
    return hours + ':' + two_digits(minutes % 60) + ':' + two_digits(seconds % 60);
}

void validate_countdown_snapshot(const CountdownSnapshot& snapshot) {
    if (snapshot.configured.value < kCountdownMinimumNanoseconds ||
        snapshot.configured.value > kCountdownMaximumNanoseconds ||
        snapshot.remaining.value < 0 || snapshot.remaining.value > snapshot.configured.value) {
        throw std::invalid_argument("Invalid configured or remaining countdown duration");
    }
    switch (snapshot.phase) {
    case CountdownPhase::Idle:
        if (snapshot.remaining.value != snapshot.configured.value || snapshot.expiry_pending) {
            throw std::invalid_argument("Idle countdown must contain full duration without pending expiry");
        }
        break;
    case CountdownPhase::Running:
    case CountdownPhase::Paused:
        if (snapshot.run_id == 0 || snapshot.remaining.value == 0 || snapshot.expiry_pending) {
            throw std::invalid_argument("Active countdown needs identity and positive remaining without pending expiry");
        }
        break;
    case CountdownPhase::Expired:
        if (snapshot.run_id == 0 || snapshot.remaining.value != 0) {
            throw std::invalid_argument("Expired countdown needs identity and zero remaining");
        }
        break;
    default:
        throw std::invalid_argument("Unknown countdown phase");
    }
}

CountdownPresentationView make_countdown_presentation(const CountdownSnapshot& snapshot) {
    validate_countdown_snapshot(snapshot);
    return {snapshot.phase, snapshot.configured, snapshot.remaining, snapshot.run_id,
            snapshot.expiry_pending, snapshot.expiry_pending,
            format_countdown_duration(snapshot.configured),
            format_countdown_duration(snapshot.remaining), status_text(snapshot)};
}
} // namespace hamclock
