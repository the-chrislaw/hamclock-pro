#pragma once

#include "domain/geometry.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace hamclock::spots {

// Provisional replacement policies, not original HamClock protocol limits.
// Text is plain ASCII, never markup/commands; adapters must not silently truncate.
inline constexpr std::uint64_t max_frequency_hz = 1'000'000'000'000ULL;
inline constexpr std::size_t max_callsign_bytes = 32;
inline constexpr std::size_t max_source_bytes = 64;
inline constexpr std::size_t max_identity_bytes = 128;
inline constexpr std::size_t max_provenance_bytes = 256;

struct FrequencyHz { std::uint64_t value; };
// Quality is supplied by the caller; this module does not verify a feed's UTC.
enum class TimeQuality { Unknown, Reported, Verified };
struct ReportedUtc {
    // Signed Unix seconds: metadata from a feed, never invented from arrival time.
    std::optional<std::int64_t> unix_seconds;
    TimeQuality quality = TimeQuality::Unknown;
};
struct SpotKey { std::string source; std::string event_identity; };
bool operator==(const SpotKey& a, const SpotKey& b) noexcept;
struct SpotEvent {
    SpotKey key;
    FrequencyHz frequency{0};
    std::string callsign;
    std::optional<std::string> spotter_callsign;
    ReportedUtc reported;
    std::optional<Location> location;
    std::string location_provenance;
};

// Validates all fields, returns an independently owned normalized copy.
// Source/identity accept nonempty ASCII alphanumeric . _ : / - tokens.
// Callsigns accept letters/digits/slash, contain an alphanumeric, uppercase output;
// no station-callsign digit rule is imposed on spot metadata.
// Provenance accepts printable ASCII (space included); present location requires
// nonempty provenance, absent location requires empty provenance.
// Reported/Verified quality requires timestamp; Unknown may carry untrusted time.
// Throws invalid_argument before any store mutation; allocation errors propagate.
SpotEvent normalize_event(const SpotEvent& event);

enum class AgeStatus { Unknown, Fresh, Stale, Future };
struct AgePolicy { std::uint64_t stale_after_seconds; std::uint64_t future_tolerance_seconds; };
struct SpotAge { AgeStatus status; std::optional<std::uint64_t> seconds; };
// Policy and now are injected. Exactly stale_after is fresh. A tolerated future
// timestamp is Fresh with zero elapsed seconds; farther future is Future with
// its positive lead. Unknown quality/time never yields a fabricated age.
// All int64 timestamp extremes are supported without signed subtraction overflow.
// Invalid quality enums or trusted quality without a timestamp throw invalid_argument.
SpotAge age_at(const ReportedUtc& time, std::int64_t now, AgePolicy policy);

} // namespace hamclock::spots
