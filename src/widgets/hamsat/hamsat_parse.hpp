#pragma once
// Parser for hams.at upcoming satellite activation alerts
// (hams.at/api/alerts/upcoming, public read endpoint). Pure functions: no
// network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::hamsat {

struct Activation {
    QString callsign;
    QString satellite;  // "SO-50"
    QString mode;       // "FM", "SSB", "CW" ...
    QString grids;      // "FN00oi" or "BL11 BL01"
    double mhz = 0;     // 0 when not announced
    QDateTime aos;      // UTC
    QDateTime los;      // UTC
    // Per-station values; hams.at fills them only for requests with an API key.
    int workable = -1;            // 1 workable, 0 not, -1 not given
    double max_elevation = -1;    // degrees; -1 when not given
    QDateTime workable_start;     // UTC; invalid when not given
    QDateTime workable_end;
    bool personalised() const { return workable >= 0; }
};

struct Feed {
    bool ok = false;
    std::vector<Activation> activations;  // soonest AOS first
};

// Rows without a callsign, satellite or readable AOS/LOS are skipped. A body
// without a "data" array yields ok == false.
Feed parse(const QByteArray& body);

// Activations whose LOS is still ahead of `now` (so one under way is kept),
// at most `limit`.
std::vector<Activation> upcoming(const Feed& feed, const QDateTime& now, int limit);

// "in 3h 12m", "in 2d 4h", "in 9m", "now" while between AOS and LOS.
QString when_text(const Activation& a, const QDateTime& now);

} // namespace hamclock::hamsat
