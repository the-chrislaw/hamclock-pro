#pragma once
// Daily and one-time alarms in UTC with injected time, so they are testable offline.
// An alarm fires when its time falls in the half-open window (previous check, now]; a clock
// jump of more than 5 minutes is not treated as elapsed time, so a time change cannot ring
// every alarm it skips over.
#include <QDateTime>
#include <QString>
#include <QVector>

namespace hamclock {

struct Alarm {
    int minute_of_day = 0;   // 0..1439, UTC
    bool daily = true;       // false: rings once, then disables itself
    bool enabled = true;
};

inline QString alarm_time_text(int minute_of_day) {
    const int m = ((minute_of_day % 1440) + 1440) % 1440;
    return QString("%1:%2 UTC").arg(m / 60, 2, 10, QChar('0')).arg(m % 60, 2, 10, QChar('0'));
}

// Next time (UTC) the alarm will ring strictly after `now`; invalid when disabled.
inline QDateTime alarm_next(const Alarm& a, const QDateTime& now) {
    if (!a.enabled) return {};
    QDateTime t(now.toUTC().date(), QTime(a.minute_of_day / 60 % 24, a.minute_of_day % 60), Qt::UTC);
    if (t <= now) t = t.addDays(1);
    return t;
}

// True when the alarm rang between prev (exclusive) and now (inclusive).
inline bool alarm_due(const Alarm& a, const QDateTime& prev, const QDateTime& now) {
    if (!a.enabled || !prev.isValid() || !now.isValid()) return false;
    const qint64 span = prev.secsTo(now);
    if (span <= 0 || span > 300) return false;
    const QDateTime next = alarm_next(a, prev);
    return next.isValid() && next <= now;
}

// "in 3h 12m" style countdown for the settings list; "now" under one minute.
inline QString alarm_in_text(const Alarm& a, const QDateTime& now) {
    const QDateTime n = alarm_next(a, now);
    if (!n.isValid()) return "off";
    const qint64 mins = (now.secsTo(n) + 59) / 60;
    if (mins < 1) return "now";
    return mins >= 60 ? QString("in %1h %2m").arg(mins / 60).arg(mins % 60) : QString("in %1m").arg(mins);
}

}  // namespace hamclock
