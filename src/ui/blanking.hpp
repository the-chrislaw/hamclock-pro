#pragma once
// Screen blanking for kiosks, in software only (a black overlay; no backlight or DPMS control).
// Blank after `idle_minutes` without a touch, and during a daily local-time window
// [off_hour, on_hour) on the chosen weekdays (a window that crosses midnight belongs to the day it starts) where a touch wakes the screen for only kWindowWakeSecs.
#include <QDateTime>

namespace hamclock {

constexpr int kWindowWakeSecs = 60;

struct BlankingConfig {
    int idle_minutes = 0;  // 0 = never blank on idle; otherwise 1..240
    int off_hour = -1;     // 0..23, -1 = no daily window
    int on_hour = -1;      // 0..23, -1 = no daily window
    int off_days = 0x7F;   // weekdays the window starts on: bit 0 = Monday .. bit 6 = Sunday
};

inline bool blanking_config_valid(const BlankingConfig& c) {
    const bool win_off = c.off_hour < 0 && c.on_hour < 0;
    const bool win_ok = win_off || (c.off_hour >= 0 && c.off_hour < 24 && c.on_hour >= 0 && c.on_hour < 24);
    return c.idle_minutes >= 0 && c.idle_minutes <= 240 && win_ok && c.off_days >= 0 && c.off_days <= 0x7F;
}

// True inside the daily window on `date`; the window may cross midnight. off == on means no window.
inline bool in_off_window(const BlankingConfig& c, const QDate& date, const QTime& local) {
    if (c.off_hour < 0 || c.on_hour < 0 || c.off_hour == c.on_hour) return false;
    const int h = local.hour();
    const bool crosses = c.off_hour > c.on_hour;
    const bool inside = crosses ? (h >= c.off_hour || h < c.on_hour) : (h >= c.off_hour && h < c.on_hour);
    if (!inside) return false;
    const QDate start = (crosses && h < c.on_hour) ? date.addDays(-1) : date;
    return (c.off_days >> (start.dayOfWeek() - 1)) & 1;
}

// Time-only form: every weekday counts (kept for callers without a date).
inline bool in_off_window(const BlankingConfig& c, const QTime& local) {
    BlankingConfig all = c;
    all.off_days = 0x7F;
    return in_off_window(all, QDate(2024, 1, 1), local);
}

inline bool should_blank(const BlankingConfig& c, const QDate& date, const QTime& local, qint64 idle_secs) {
    if (idle_secs < 0) idle_secs = 0;
    if (c.idle_minutes > 0 && idle_secs >= qint64(c.idle_minutes) * 60) return true;
    return in_off_window(c, date, local) && idle_secs >= kWindowWakeSecs;
}

inline bool should_blank(const BlankingConfig& c, const QTime& local, qint64 idle_secs) {
    BlankingConfig all = c;
    all.off_days = 0x7F;
    return should_blank(all, QDate(2024, 1, 1), local, idle_secs);
}

}  // namespace hamclock
