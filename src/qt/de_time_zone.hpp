#pragma once

#include "../domain/clock_state.hpp"
#include <optional>
#include <string>

namespace hamclock {
// True when the OS tz database knows this IANA id (e.g. "America/New_York").
bool is_valid_de_zone_id(const std::string& iana_id);

// UTC offset in effect for the zone at the given actual UTC instant, including
// daylight saving, from the OS tz database via QTimeZone. No hand-rolled DST
// rules. Unknown or empty zone id, or an unrepresentable instant -> nullopt;
// callers must show DE time as unavailable, never fall back to host-local time.
std::optional<UtcOffsetSeconds> resolve_de_zone_offset(const std::string& iana_id, UtcSeconds utc);
} // namespace hamclock
