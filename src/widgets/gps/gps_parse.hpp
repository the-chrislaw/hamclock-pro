#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <optional>

namespace hamclock::gps {

struct Fix {
    int mode = 0; // gpsd: 0/1 no fix, 2 = 2D, 3 = 3D
    std::optional<double> lat, lon, alt_m;
    std::optional<QDateTime> time;  // UTC fix time from a TPV "time" field, when present
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

// "Clock agrees with GPS (within 2 s)" / "Clock is 12 s ahead of GPS". gpsd reports
// a fix some hundreds of ms after it was taken, so differences under 2 s are not
// resolved and never shown as a precise offset.
QString clock_vs_gps_text(const QDateTime& gps_utc, const QDateTime& system_utc);

} // namespace hamclock::gps
