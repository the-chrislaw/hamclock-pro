#pragma once
// Parser for NOAA SWPC's R/S/G space weather scales (products/noaa-scales.json,
// public US-government data, no key). Pure functions over the raw body.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <array>

namespace hamclock::solarscales {

struct Day {
    int r = -1, s = -1, g = -1;          // scale 0-5 where SWPC states one, else -1
    int r_minor = -1, r_major = -1;      // forecast probability (%) of R1-R2 / R3+ blackouts, -1 if absent
    int s_prob = -1;                     // forecast probability (%) of S1+ radiation storm
};

struct Scales {
    bool valid = false;
    QDateTime observed;                  // UTC, time of the "current" record
    Day current;                         // R/S/G now
    Day past24h;                         // maxima over the last 24 h
    std::array<Day, 3> forecast;         // today (rest of day), tomorrow, day after
};

// valid=false when the body is not the expected object or lacks the current record.
Scales parse_scales(const QByteArray& body);

// "Minor", "Moderate", "Strong", "Severe", "Extreme"; empty for 0 or out of range.
QString scale_word(int level);

} // namespace hamclock::solarscales
