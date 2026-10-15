#pragma once
#include <QByteArray>
#include <QString>
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

// aircraft.json URL from options.ini ([adsb] host / port). A host must be a
// plain name or address (letters, digits, '.', '-', ':'), a port 1..65535;
// each invalid field falls back to localhost:8080. IPv6 literals get brackets.
QString aircraft_url(const QString& host, const QString& port);

} // namespace hamclock::adsb
