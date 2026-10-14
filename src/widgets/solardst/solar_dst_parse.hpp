#pragma once
// Parser for the Kyoto Dst index as republished by NOAA SWPC (products/kyoto-dst.json,
// public data, no key): hourly records {time_tag, dst}. Pure functions over the raw body.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::solardst {

struct Sample {
    QDateTime time;  // UTC, start of the hour
    int dst = 0;     // nT
};

// Oldest first; records with a missing time or value are skipped. Empty when the body is not the expected array.
std::vector<Sample> parse_dst(const QByteArray& body);

// "Quiet" (> -30 nT), "Weak" (-30..-50), "Moderate" (-50..-100), "Intense" (-100..-250), "Super" (< -250).
QString dst_word(int dst);

} // namespace hamclock::solardst
