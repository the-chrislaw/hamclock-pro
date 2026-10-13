#pragma once

#include "big_clock_civil_adapter.hpp"
#include "de_time_zone.hpp"
#include <optional>
#include <string>

namespace hamclock {
enum class DeClockZoneCivilStatus { Ready, MissingSource, SourceOutOfRange, UnknownZone, ConversionFailed };
struct DeClockZoneCivilResult {
    DeClockZoneCivilStatus status;
    // Offset in effect at the instant (incl. DST), present iff the zone resolved.
    std::optional<UtcOffsetSeconds> offset;
    // Local civil fields iff Ready.
    std::optional<BigClockCivilTime> civil;
};

// DE local civil time for an IANA zone at an actual UTC instant. The offset
// comes from the OS tz database (resolve_de_zone_offset); an unknown zone or
// missing source yields no civil fields, never host-local time.
DeClockZoneCivilResult adapt_de_clock_zone_civil(const std::string& iana_id, std::optional<UtcSeconds> actual);
} // namespace hamclock
