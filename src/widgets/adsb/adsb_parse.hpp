#pragma once
#include <QByteArray>
#include <optional>

namespace hamclock::adsb {

struct Summary {
    double now = 0;       // receiver clock, Unix seconds
    int tracked = 0;      // aircraft heard within the last 60 s
    int withPosition = 0; // of those, with a latitude/longitude
};

// Parses dump1090/PiAware aircraft.json. Garbage, a missing "now" or a
// missing "aircraft" array yield nullopt, never an empty-but-live summary.
std::optional<Summary> parse_aircraft(const QByteArray& json);

} // namespace hamclock::adsb
