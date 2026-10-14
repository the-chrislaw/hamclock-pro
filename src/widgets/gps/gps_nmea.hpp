#pragma once
#include "gps_parse.hpp"
#include <QByteArray>
#include <QDateTime>
#include <QList>
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

// Splits a raw serial byte stream into sentences. Partial lines are kept until
// their newline arrives; a line longer than 128 bytes (line noise, wrong baud
// rate) is dropped instead of buffered without bound.
class NmeaStream {
public:
    QList<QByteArray> feed(const QByteArray& chunk);
private:
    QByteArray buf_;
    bool overflow_ = false;
};

// Merges GGA and RMC into one picture of the receiver. A position is only
// reported while the last fix is younger than max_age_s, so a receiver that
// goes quiet or loses its fix is never shown as live.
class NmeaTracker {
public:
    explicit NmeaTracker(int max_age_s = 5) : max_age_s_(max_age_s) {}
    void apply(const Nmea& n, qint64 now_ms);
    bool fresh(qint64 now_ms) const;
    Fix fix(qint64 now_ms) const { return fresh(now_ms) ? fix_ : Fix{}; }
    int satellites() const { return used_; }
    // UTC from the last RMC, advanced by the time since it arrived.
    std::optional<QDateTime> utc(qint64 now_ms) const;
private:
    int max_age_s_;
    Fix fix_;
    int used_ = 0;
    qint64 fix_ms_ = -1, utc_ms_ = -1;
    QDateTime utc_;
};

} // namespace hamclock::gps
