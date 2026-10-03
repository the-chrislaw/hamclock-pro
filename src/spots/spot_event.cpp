#include "spots/spot_event.hpp"
#include <stdexcept>

namespace hamclock::spots {
namespace {
bool alnum(unsigned char c) noexcept {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}
void token(const std::string& text, std::size_t limit) {
    if (text.empty() || text.size() > limit) throw std::invalid_argument("invalid spot identity length");
    for (unsigned char c : text)
        if (!alnum(c) && c != '.' && c != '_' && c != ':' && c != '/' && c != '-')
            throw std::invalid_argument("invalid spot identity character");
}
std::string call(const std::string& text) {
    if (text.empty() || text.size() > max_callsign_bytes) throw std::invalid_argument("invalid spot callsign length");
    bool has_alnum = false;
    std::string result(text);
    for (char& raw : result) {
        const auto c = static_cast<unsigned char>(raw);
        if (!alnum(c) && c != '/') throw std::invalid_argument("invalid spot callsign character");
        has_alnum = has_alnum || alnum(c);
        if (c >= 'a' && c <= 'z') raw = static_cast<char>(c - 'a' + 'A');
    }
    if (!has_alnum) throw std::invalid_argument("invalid spot callsign");
    return result;
}
void quality(const ReportedUtc& time) {
    switch (time.quality) {
    case TimeQuality::Unknown: break;
    case TimeQuality::Reported:
    case TimeQuality::Verified:
        if (!time.unix_seconds) throw std::invalid_argument("spot UTC quality requires timestamp");
        break;
    default: throw std::invalid_argument("invalid spot time quality");
    }
}
std::uint64_t distance(std::int64_t later, std::int64_t earlier) noexcept {
    // Unsigned modular subtraction gives the exact nonnegative difference when
    // signed ordering establishes later >= earlier, even across INT64_MIN/0.
    return static_cast<std::uint64_t>(later) - static_cast<std::uint64_t>(earlier);
}
} // namespace

bool operator==(const SpotKey& a, const SpotKey& b) noexcept {
    return a.source == b.source && a.event_identity == b.event_identity;
}
SpotEvent normalize_event(const SpotEvent& event) {
    token(event.key.source, max_source_bytes);
    token(event.key.event_identity, max_identity_bytes);
    if (event.frequency.value == 0 || event.frequency.value > max_frequency_hz)
        throw std::invalid_argument("spot frequency outside replacement bounds");
    quality(event.reported);
    if (event.location && !valid_location(*event.location)) throw std::invalid_argument("invalid spot location");
    if (event.location_provenance.size() > max_provenance_bytes)
        throw std::invalid_argument("spot provenance too long");
    for (unsigned char c : event.location_provenance)
        if (c < 32 || c > 126) throw std::invalid_argument("invalid spot provenance character");
    if (event.location.has_value() == event.location_provenance.empty())
        throw std::invalid_argument("spot location requires matching provenance");
    // Check every text bound before copying the aggregate (including call fields).
    auto normalized_call = call(event.callsign);
    std::optional<std::string> normalized_spotter;
    if (event.spotter_callsign) normalized_spotter = call(*event.spotter_callsign);
    SpotEvent result(event);
    result.callsign = std::move(normalized_call);
    result.spotter_callsign = std::move(normalized_spotter);
    return result;
}
SpotAge age_at(const ReportedUtc& time, std::int64_t now, AgePolicy policy) {
    quality(time);
    if (time.quality == TimeQuality::Unknown || !time.unix_seconds)
        return {AgeStatus::Unknown, std::nullopt};
    if (*time.unix_seconds > now) {
        const auto lead = distance(*time.unix_seconds, now);
        if (lead > policy.future_tolerance_seconds) return {AgeStatus::Future, lead};
        return {AgeStatus::Fresh, 0};
    }
    const auto elapsed = distance(now, *time.unix_seconds);
    return {elapsed > policy.stale_after_seconds ? AgeStatus::Stale : AgeStatus::Fresh, elapsed};
}
} // namespace hamclock::spots
