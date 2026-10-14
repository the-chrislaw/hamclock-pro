#pragma once
#include <QByteArray>
#include <optional>

namespace hamclock::gps {

struct Fix {
    int mode = 0; // gpsd: 0/1 no fix, 2 = 2D, 3 = 3D
    std::optional<double> lat, lon, alt_m;
    bool has_position() const { return mode >= 2 && lat && lon; }
};

struct Sky {
    int used = 0, seen = 0;
};

struct Message {
    enum Kind { Other, Tpv, SkyReport } kind = Other;
    Fix fix;
    Sky sky;
};

// One line of gpsd's JSON stream. Malformed input or implausible coordinates
// yield kind Other / no position, never a bogus fix.
Message parse_line(const QByteArray& line);

} // namespace hamclock::gps
