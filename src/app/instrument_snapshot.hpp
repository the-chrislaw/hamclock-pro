#pragma once

#include "instrument_catalog.hpp"
#include "../domain/clock_state.hpp"
#include <cstdint>
#include <optional>
#include <string>

namespace hamclock {

// Bounded latest-reading preparation contract, not a full history/chart or
// provider schema. Units identify supplied data; no implicit conversion occurs.
enum class InstrumentUnit : std::uint8_t { Dimensionless, Sfu, Nanotesla, WattsPerSquareMetre,
    KilometresPerSecond, ProtonsPerCubicCentimetre, Megahertz, Percent,
    Celsius, Hectopascal };
enum class ReadingQuality : std::uint8_t { Valid, Missing, Invalid };
struct InstrumentReading {
    UtcSeconds observed_at{0};
    std::optional<double> value;
    InstrumentUnit unit = InstrumentUnit::Dimensionless;
    ReadingQuality quality = ReadingQuality::Missing;
};
struct InstrumentForecast {
    UtcSeconds issued_at{0};
    UtcSeconds valid_at{0};
    double value = 0;
    InstrumentUnit unit = InstrumentUnit::Dimensionless;
};
struct InstrumentProvenance {
    // Plain printable ASCII only, nonempty; limits respectively 64/96/256 bytes.
    // Caller-supplied descriptive provenance does not certify source rights,
    // accuracy or authenticity. Metadata is never interpreted as an address
    // or executable input; callers must keep secrets out of these labels.
    std::string provider_id;
    std::string source_revision;
    std::string attribution;
};
struct InstrumentSnapshot {
    InstrumentId instrument_id;
    InstrumentProvenance provenance;
    UtcSeconds fetched_at;
    MonotonicNanoseconds retrieved_at;
    // Caller supplies deadline from its own policy; this model invents no
    // cadence, HTTP pacing or provider-specific timeout/retry permission.
    MonotonicNanoseconds refresh_deadline;
    std::optional<InstrumentReading> observation;
    std::optional<InstrumentForecast> forecast;
};

enum class InstrumentPhase : std::uint8_t { Unavailable, Loading, Ready, Offline, Error,
    Disabled, Unsupported };
enum class InstrumentStatus : std::uint8_t { Unavailable, Loading, Current, Stale,
    OfflineCached, Error, Disabled, Unsupported };
enum class InstrumentStatusReason : std::uint8_t { NoObservation, Loading, Fresh, AgeExpired,
    RefreshDue, FutureTimestamp, MonotonicBeforeRetrieval, Offline,
    FetchError, Disabled, Unsupported, Unavailable, MissingReading, InvalidReading };
struct ObservationAgeSeconds { std::int64_t value; };
struct InstrumentFreshnessPolicy {
    // Explicit per-product observation policy. Nonnegative, inclusive age;
    // zero accepts only an observation at actual_utc. No universal default.
    ObservationAgeSeconds maximum_observation_age;
};
struct InstrumentEvaluation {
    InstrumentStatus status;
    InstrumentStatusReason reason;
    // Full uint64 difference handles opposite signed64 UTC endpoints.
    // Absent for missing/future/no observation; zero is a valid age/value.
    std::optional<std::uint64_t> observation_age_seconds;
    bool observation_usable = false;
    // Forecast is explicitly separate and never promotes status to Current.
    bool forecast_present = false;
};

// Validates known IDs/enums, finite readings, explicit missing quality, ASCII
// bounds, issue<=valid and deadline>=retrieval. Valid quality requires a finite
// value; Missing/Invalid quality requires no value. Throws invalid_argument on bad
// input. Does not certify family-specific units/ranges or source permissions.
void validate_instrument_snapshot(const InstrumentSnapshot&);

// Actual UTC only: display/preview clock offsets must not be injected here.
// For enabled phases, future fetched/observed/forecast-issued timestamps or
// reversed monotonic evidence => Error, no usable observation/forecast. This
// also applies to a future timestamp on Missing/Invalid-quality observations.
// Forecast validity can legitimately be in the future.
// Ready => Current only for valid fresh observation before refresh_deadline;
// deadline equality => Stale. Error/Loading with usable cached observation =>
// Stale; Offline with usable observation => OfflineCached even if age-expired,
// preserving explicit age/reason rather than portraying it as current.
// Disabled/Unsupported/Unavailable hide readings. Missing/invalid quality is
// never replaced by zero. Phase, policy and snapshot schema are validated even
// when the snapshot is absent or the phase hides data. Negative policy throws
// invalid_argument. Missing quality => Unavailable, Invalid quality => Error.
InstrumentEvaluation evaluate_instrument_snapshot(
    const std::optional<InstrumentSnapshot>&, InstrumentPhase,
    UtcSeconds actual_utc, MonotonicNanoseconds now,
    InstrumentFreshnessPolicy);

// Synchronous in-memory cache only, with strong rejection/exception guarantee.
// No provider access, scheduling, system-clock mutation, UI, persistence or
// hardware. Async selection/generation guards require a later adapter contract.
class InstrumentSnapshotState {
public:
    explicit InstrumentSnapshotState(InstrumentId);
    InstrumentId instrument_id() const noexcept;
    const std::optional<InstrumentSnapshot>& snapshot() const noexcept;
    InstrumentPhase phase() const noexcept;
    // Schema validation + matching ID + temporary copy precede replacement.
    // Successful acceptance marks Ready; failure leaves cache/phase untouched.
    void accept(const InstrumentSnapshot&);
    // Error/offline/loading preserve the previous cache. Invalid enum rejected
    // atomically. Unavailable/Disabled/Unsupported do not erase stored data.
    void set_phase(InstrumentPhase);
    InstrumentEvaluation evaluate(UtcSeconds actual_utc,
        MonotonicNanoseconds now, InstrumentFreshnessPolicy) const;
private:
    InstrumentId id_;
    InstrumentPhase phase_ = InstrumentPhase::Unavailable;
    std::optional<InstrumentSnapshot> snapshot_;
};
} // namespace hamclock
