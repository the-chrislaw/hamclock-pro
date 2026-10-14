#pragma once
// Auxiliary time line under the station clock: tap cycles
// day of year -> Julian date -> MJD -> local sidereal time -> UNIX time.
#include <QDateTime>
#include <QString>
#include <cmath>

namespace hamclock {

constexpr int kAuxTimeModes = 5;

inline int next_aux_time_mode(int mode) { return ((mode % kAuxTimeModes) + kAuxTimeModes + 1) % kAuxTimeModes; }

// Local mean sidereal time in hours [0, 24) at an east-positive longitude (Meeus ch. 12).
inline double local_sidereal_hours(qint64 unix_secs, double longitude_deg) {
    const double jd = static_cast<double>(unix_secs) / 86400.0 + 2440587.5;
    const double deg = 280.46061837 + 360.98564736629 * (jd - 2451545.0) + longitude_deg;
    return std::fmod(std::fmod(deg, 360.0) + 360.0, 360.0) / 15.0;
}

// Week of the year. Monday start: ISO 8601. Sunday start: US style, week 1 is the week
// (Sunday..Saturday) containing 1 January.
inline int week_of_year(const QDate& d, bool sunday_start) {
    if (!sunday_start) return d.weekNumber();
    const int jan1_dow = QDate(d.year(), 1, 1).dayOfWeek() % 7;  // Sunday = 0
    return (d.dayOfYear() - 1 + jan1_dow) / 7 + 1;
}

inline QString aux_time_text(int mode, qint64 unix_secs, double longitude_deg, bool sunday_week = false) {
    const double jd = static_cast<double>(unix_secs) / 86400.0 + 2440587.5;
    switch (((mode % kAuxTimeModes) + kAuxTimeModes) % kAuxTimeModes) {
    case 1: return QStringLiteral("JD %1").arg(jd, 0, 'f', 5);
    case 2: return QStringLiteral("MJD %1").arg(jd - 2400000.5, 0, 'f', 5);
    case 3: {
        int tenths = static_cast<int>(std::floor(local_sidereal_hours(unix_secs, longitude_deg) * 3600.0));
        tenths %= 86400;
        return QStringLiteral("LST %1:%2:%3")
            .arg(tenths / 3600, 2, 10, QLatin1Char('0'))
            .arg(tenths / 60 % 60, 2, 10, QLatin1Char('0'))
            .arg(tenths % 60, 2, 10, QLatin1Char('0'));
    }
    case 4: return QStringLiteral("UNIX %1").arg(unix_secs);
    default: {
        const auto d = QDateTime::fromSecsSinceEpoch(unix_secs, Qt::UTC).date();
        return QStringLiteral("Day %1 of %2 · week %3").arg(d.dayOfYear()).arg(d.daysInYear()).arg(week_of_year(d, sunday_week));
    }
    }
}

}  // namespace hamclock
