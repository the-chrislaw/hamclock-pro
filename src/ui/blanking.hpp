#pragma once
// Screen blanking for kiosks, in software only (a black overlay; no backlight or DPMS control).
// Blank after `idle_minutes` without a touch, and during a daily local-time window
// [off_hour, on_hour) where a touch wakes the screen for only kWindowWakeSecs.
#include <QDateTime>

namespace hamclock {

constexpr int kWindowWakeSecs = 60;

struct BlankingConfig {
    int idle_minutes = 0;  // 0 = never blank on idle; otherwise 1..240
    int off_hour = -1;     // 0..23, -1 = no daily window
    int on_hour = -1;      // 0..23, -1 = no daily window
};

inline bool blanking_config_valid(const BlankingConfig& c) {
    const bool win_off = c.off_hour < 0 && c.on_hour < 0;
    const bool win_ok = win_off || (c.off_hour >= 0 && c.off_hour < 24 && c.on_hour >= 0 && c.on_hour < 24);
    return c.idle_minutes >= 0 && c.idle_minutes <= 240 && win_ok;
}

// True inside the daily window; the window may cross midnight. off == on means no window.
inline bool in_off_window(const BlankingConfig& c, const QTime& local) {
    if (c.off_hour < 0 || c.on_hour < 0 || c.off_hour == c.on_hour) return false;
    const int h = local.hour();
    return c.off_hour < c.on_hour ? (h >= c.off_hour && h < c.on_hour) : (h >= c.off_hour || h < c.on_hour);
}

inline bool should_blank(const BlankingConfig& c, const QTime& local, qint64 idle_secs) {
    if (idle_secs < 0) idle_secs = 0;
    if (c.idle_minutes > 0 && idle_secs >= qint64(c.idle_minutes) * 60) return true;
    return in_off_window(c, local) && idle_secs >= kWindowWakeSecs;
}

}  // namespace hamclock
