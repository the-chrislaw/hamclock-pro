#pragma once
#include "gps_parse.hpp"
#include <QByteArray>
#include <QDateTime>
#include <optional>

namespace hamclock::gps {

// One NMEA 0183 sentence from a serial GPS receiver. Only GGA (position, fix
// quality, satellites used) and RMC (validity, position, UTC date and time) are
// understood; anything else, a bad checksum or implausible values give kind Other.
struct Nmea {
    enum Kind { Other, Gga, Rmc } kind = Other;
    Fix fix;                          // mode 3 for GGA with altitude, 2 otherwise; 0 when no fix
    int used = 0;                     // GGA satellites in use
    std::optional<QDateTime> utc;     // RMC, only with status A and a valid date
};

Nmea parse_nmea(const QByteArray& sentence);

} // namespace hamclock::gps
