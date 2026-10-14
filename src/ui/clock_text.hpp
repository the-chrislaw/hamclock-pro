#pragma once

#include <QDateTime>
#include <QString>

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

} // namespace hamclock
