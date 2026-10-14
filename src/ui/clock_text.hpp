#pragma once

#include <QDateTime>
#include <QString>

namespace hamclock {

// Clock text for an instant shifted by a UTC offset (seconds). The shifted
// QDateTime is only used for formatting, so build it as UTC.
inline QString clock_time(const QDateTime& utc, qint64 offset_secs) {
    return utc.addSecs(offset_secs).toString("HH:mm:ss");
}
inline QString clock_date(const QDateTime& utc, qint64 offset_secs) {
    return utc.addSecs(offset_secs).toString("ddd d MMM yyyy");
}

} // namespace hamclock
