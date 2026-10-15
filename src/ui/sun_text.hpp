#pragma once
// Compact next-sunrise/sunset text for the DE and DX boxes.
#include "../domain/astro_sun.hpp"
#include <QString>
#include <cstdint>

namespace hamclock {

inline QString sun_countdown(std::int64_t seconds) {
    const std::int64_t minutes = seconds < 0 ? 0 : (seconds + 30) / 60;
    if (minutes < 60) return QString("in %1m").arg(minutes);
    if (minutes < 24 * 60) return QString("in %1h %2m").arg(minutes / 60).arg(minutes % 60);
    return QString("in %1d %2h").arg(minutes / (24 * 60)).arg(minutes % (24 * 60) / 60);
}

// "06:02z" for the "at" display mode (UTC, never the viewer's zone).
inline QString sun_clock(std::int64_t utc) {
    const std::int64_t m = (utc % 86400 + 86400) % 86400 / 60;
    return QString("%1:%2z").arg(m / 60, 2, 10, QChar('0')).arg(m % 60, 2, 10, QChar('0'));
}

// "Rise in 3h 12m · Set in 14h 5m" (soonest first). Polar days say so. Empty
// when the coordinates are out of range. `at` shows UTC clock times instead.
inline QString sun_line(std::int64_t now_utc, double latitude, double longitude, bool at = false) {
    try {
        const auto rise = next_sun_event({now_utc}, SunEventKind::Rise, latitude, longitude);
        const auto set = next_sun_event({now_utc}, SunEventKind::Set, latitude, longitude);
        if (!rise && !set) {
            const auto day = sun_events_for_utc_day(now_utc - now_utc % 86400, latitude, longitude);
            return day.polar == PolarState::MidnightSun ? "Sun up all day"
                 : day.polar == PolarState::PolarNight ? "Sun down all day" : QString();
        }
        const QString r = rise ? "Rise " + (at ? sun_clock(rise->value) : sun_countdown(rise->value - now_utc)) : QString();
        const QString s = set ? "Set " + (at ? sun_clock(set->value) : sun_countdown(set->value - now_utc)) : QString();
        if (!rise) return s;
        if (!set) return r;
        return rise->value <= set->value ? r + " · " + s : s + " · " + r;
    } catch (const std::exception&) {
        return {};
    }
}

} // namespace hamclock
