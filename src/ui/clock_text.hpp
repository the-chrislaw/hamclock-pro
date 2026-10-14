#pragma once

#include <QDateTime>
#include <QString>
#include <QTimeZone>

namespace hamclock {

// Date order for the station clocks: 0 = "Thu 15 Oct 2026", 1 = "Thu Oct 15 2026",
// 2 = "2026-10-15 Thu". Unknown values use 0.
inline QString date_format(int order) {
    return order == 1 ? "ddd MMM d yyyy" : order == 2 ? "yyyy-MM-dd ddd" : "ddd d MMM yyyy";
}

// Clock text for an instant shifted by a UTC offset (seconds). The shifted
// QDateTime is only used for formatting, so build it as UTC. 12 h mode adds AM/PM.
inline QString clock_time(const QDateTime& utc, qint64 offset_secs, bool h12 = false) {
    return utc.addSecs(offset_secs).toString(h12 ? "h:mm:ss AP" : "HH:mm:ss");
}
inline QString clock_date(const QDateTime& utc, qint64 offset_secs, int order = 0) {
    return utc.addSecs(offset_secs).toString(date_format(order));
}

// "Local 14:05 · Tokyo (zone from coordinates)" for an IANA zone id; empty when the
// id is empty or unknown. `suggested` marks a zone guessed from coordinates.
inline QString zone_local_text(const QString& zone_id, const QDateTime& utc, bool h12 = false, bool suggested = true) {
    if (zone_id.isEmpty()) return {};
    const QTimeZone tz(zone_id.toUtf8());
    if (!tz.isValid()) return {};
    QString city = zone_id.mid(zone_id.lastIndexOf('/') + 1);
    city.replace('_', ' ');
    const QString time = utc.addSecs(tz.offsetFromUtc(utc)).toString(h12 ? "h:mm AP" : "HH:mm");
    return "Local " + time + " · " + city + (suggested ? " (auto)" : "");
}

} // namespace hamclock
