#include "instrument_snapshot.hpp"

#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hamclock {
namespace {
bool valid_unit(InstrumentUnit unit) noexcept {
    switch (unit) {
    case InstrumentUnit::Dimensionless: case InstrumentUnit::Sfu:
    case InstrumentUnit::Nanotesla: case InstrumentUnit::WattsPerSquareMetre:
    case InstrumentUnit::KilometresPerSecond: case InstrumentUnit::ProtonsPerCubicCentimetre:
    case InstrumentUnit::Megahertz: case InstrumentUnit::Percent:
    case InstrumentUnit::Celsius: case InstrumentUnit::Hectopascal: return true;
    }
    return false;
}
bool valid_phase(InstrumentPhase phase) noexcept {
    switch (phase) {
    case InstrumentPhase::Unavailable: case InstrumentPhase::Loading:
    case InstrumentPhase::Ready: case InstrumentPhase::Offline:
    case InstrumentPhase::Error: case InstrumentPhase::Disabled:
    case InstrumentPhase::Unsupported: return true;
    }
    return false;
}
void require(bool condition, const char* message) {
    if (!condition) { throw std::invalid_argument(message); }
}
void validate_text(const std::string& text, std::size_t maximum) {
    require(!text.empty() && text.size() <= maximum, "invalid provenance length");
    for (const unsigned char c : text) {
        require(c >= 32 && c <= 126, "provenance must be printable ASCII");
    }
}
InstrumentEvaluation result(InstrumentStatus status, InstrumentStatusReason reason,
    std::optional<std::uint64_t> age = std::nullopt, bool usable = false,
    bool forecast = false) noexcept {
    return {status, reason, age, usable, forecast};
}
} // namespace

void validate_instrument_snapshot(const InstrumentSnapshot& snapshot) {
    require(find_instrument(snapshot.instrument_id) != nullptr, "unknown instrument ID");
    validate_text(snapshot.provenance.provider_id, 64);
    validate_text(snapshot.provenance.source_revision, 96);
    validate_text(snapshot.provenance.attribution, 256);
    require(snapshot.refresh_deadline.value >= snapshot.retrieved_at.value,
            "refresh deadline before retrieval");
    if (snapshot.observation) {
        const auto& observation = *snapshot.observation;
        require(valid_unit(observation.unit), "unknown observation unit");
        switch (observation.quality) {
        case ReadingQuality::Valid:
            require(observation.value.has_value() && std::isfinite(*observation.value),
                    "valid reading requires finite value");
            break;
        case ReadingQuality::Missing: case ReadingQuality::Invalid:
            require(!observation.value.has_value(), "unusable reading must have no value");
            break;
        default: throw std::invalid_argument("unknown reading quality");
        }
    }
    if (snapshot.forecast) {
        const auto& forecast = *snapshot.forecast;
        require(valid_unit(forecast.unit), "unknown forecast unit");
        require(std::isfinite(forecast.value), "forecast requires finite value");
        require(forecast.issued_at.value <= forecast.valid_at.value,
                "forecast issue after validity");
    }
}

InstrumentEvaluation evaluate_instrument_snapshot(
    const std::optional<InstrumentSnapshot>& snapshot, InstrumentPhase phase,
    UtcSeconds actual_utc, MonotonicNanoseconds now,
    InstrumentFreshnessPolicy policy) {
    require(valid_phase(phase), "unknown instrument phase");
    require(policy.maximum_observation_age.value >= 0, "negative observation age policy");
    if (snapshot) { validate_instrument_snapshot(*snapshot); }
    switch (phase) {
    case InstrumentPhase::Unavailable:
        return result(InstrumentStatus::Unavailable, InstrumentStatusReason::Unavailable);
    case InstrumentPhase::Disabled:
        return result(InstrumentStatus::Disabled, InstrumentStatusReason::Disabled);
    case InstrumentPhase::Unsupported:
        return result(InstrumentStatus::Unsupported, InstrumentStatusReason::Unsupported);
    default: break;
    }
    if (snapshot) {
        if (snapshot->fetched_at.value > actual_utc.value ||
            (snapshot->observation && snapshot->observation->observed_at.value > actual_utc.value) ||
            (snapshot->forecast && snapshot->forecast->issued_at.value > actual_utc.value)) {
            return result(InstrumentStatus::Error, InstrumentStatusReason::FutureTimestamp);
        }
        if (now.value < snapshot->retrieved_at.value) {
            return result(InstrumentStatus::Error, InstrumentStatusReason::MonotonicBeforeRetrieval);
        }
    }
    const bool forecast = snapshot && snapshot->forecast.has_value();
    const bool observation = snapshot && snapshot->observation.has_value();
    if (observation && snapshot->observation->quality == ReadingQuality::Invalid) {
        return result(InstrumentStatus::Error, InstrumentStatusReason::InvalidReading,
                      std::nullopt, false, forecast);
    }
    if (!observation || snapshot->observation->quality == ReadingQuality::Missing) {
        switch (phase) {
        case InstrumentPhase::Loading:
            return result(InstrumentStatus::Loading, InstrumentStatusReason::Loading,
                          std::nullopt, false, forecast);
        case InstrumentPhase::Error:
            return result(InstrumentStatus::Error, InstrumentStatusReason::FetchError,
                          std::nullopt, false, forecast);
        case InstrumentPhase::Offline:
            return result(InstrumentStatus::Unavailable, InstrumentStatusReason::Offline,
                          std::nullopt, false, forecast);
        default:
            return result(InstrumentStatus::Unavailable, observation ?
                InstrumentStatusReason::MissingReading : InstrumentStatusReason::NoObservation,
                std::nullopt, false, forecast);
        }
    }
    // Ordering was checked above. Unsigned subtraction represents the full
    // nonnegative signed64-endpoint distance without signed overflow.
    const auto age = static_cast<std::uint64_t>(actual_utc.value) -
        static_cast<std::uint64_t>(snapshot->observation->observed_at.value);
    switch (phase) {
    case InstrumentPhase::Offline:
        return result(InstrumentStatus::OfflineCached, InstrumentStatusReason::Offline,
                      age, true, forecast);
    case InstrumentPhase::Error:
        return result(InstrumentStatus::Stale, InstrumentStatusReason::FetchError,
                      age, true, forecast);
    case InstrumentPhase::Loading:
        return result(InstrumentStatus::Stale, InstrumentStatusReason::Loading,
                      age, true, forecast);
    default: break;
    }
    if (age > static_cast<std::uint64_t>(policy.maximum_observation_age.value)) {
        return result(InstrumentStatus::Stale, InstrumentStatusReason::AgeExpired,
                      age, true, forecast);
    }
    if (now.value >= snapshot->refresh_deadline.value) {
        return result(InstrumentStatus::Stale, InstrumentStatusReason::RefreshDue,
                      age, true, forecast);
    }
    return result(InstrumentStatus::Current, InstrumentStatusReason::Fresh,
                  age, true, forecast);
}

InstrumentSnapshotState::InstrumentSnapshotState(InstrumentId id) : id_(id) {
    require(find_instrument(id) != nullptr, "unknown instrument state ID");
}
InstrumentId InstrumentSnapshotState::instrument_id() const noexcept { return id_; }
const std::optional<InstrumentSnapshot>& InstrumentSnapshotState::snapshot() const noexcept {
    return snapshot_;
}
InstrumentPhase InstrumentSnapshotState::phase() const noexcept { return phase_; }
void InstrumentSnapshotState::accept(const InstrumentSnapshot& input) {
    validate_instrument_snapshot(input);
    require(input.instrument_id == id_, "snapshot does not match instrument state");
    std::optional<InstrumentSnapshot> candidate(input);
    static_assert(std::is_nothrow_swappable_v<InstrumentSnapshot>,
                  "cache replacement must not throw after copy");
    static_assert(noexcept(snapshot_.swap(candidate)),
                  "optional cache replacement must not throw");
    snapshot_.swap(candidate);
    phase_ = InstrumentPhase::Ready;
}
void InstrumentSnapshotState::set_phase(InstrumentPhase phase) {
    require(valid_phase(phase), "unknown instrument phase");
    phase_ = phase;
}
InstrumentEvaluation InstrumentSnapshotState::evaluate(UtcSeconds actual_utc,
    MonotonicNanoseconds now, InstrumentFreshnessPolicy policy) const {
    return evaluate_instrument_snapshot(snapshot_, phase_, actual_utc, now, policy);
}
} // namespace hamclock
