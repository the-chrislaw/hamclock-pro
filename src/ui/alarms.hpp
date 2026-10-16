#pragma once
// Daily and one-time alarms in UTC with injected time, so they are testable offline.
// An alarm fires when its time falls in the half-open window (previous check, now]; a clock
// jump of more than 5 minutes is not treated as elapsed time, so a time change cannot ring
// every alarm it skips over.
#include <QDateTime>
#include <QString>
#include <QStringList>
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

// Persistence text: "HHMM,d|o,1|0" per alarm (UTC). Invalid entries are dropped, at most 8 kept.
inline QStringList alarms_serialize(const QVector<Alarm>& alarms) {
    QStringList out;
    for (const Alarm& a : alarms)
        out << QString("%1%2,%3,%4").arg(a.minute_of_day / 60, 2, 10, QChar('0')).arg(a.minute_of_day % 60, 2, 10, QChar('0'))
                   .arg(a.daily ? "d" : "o").arg(a.enabled ? 1 : 0);
    return out;
}

inline QVector<Alarm> alarms_parse(const QStringList& lines) {
    QVector<Alarm> out;
    for (const QString& line : lines) {
        const QStringList f = line.trimmed().split(',');
        if (f.size() != 3 || f[0].size() != 4 || (f[1] != "d" && f[1] != "o") || (f[2] != "0" && f[2] != "1")) continue;
        bool ok_h = false, ok_m = false;
        const int h = f[0].left(2).toInt(&ok_h), m = f[0].mid(2).toInt(&ok_m);
        if (!ok_h || !ok_m || h < 0 || h > 23 || m < 0 || m > 59) continue;
        Alarm a;
        a.minute_of_day = h * 60 + m;
        a.daily = f[1] == "d";
        a.enabled = f[2] == "1";
        out.push_back(a);
        if (out.size() >= 8) break;
    }
    return out;
}

}  // namespace hamclock
