#pragma once
// Display choices a new install takes from the device region (units, clock, date, week).
#include <QLocale>
#include <QString>

namespace hamclock {

struct RegionDefaults {
    bool imperial = false;            // distances in miles
    bool clock12h = false;            // AM/PM clock
    int date_order = 0;               // 0 "Thu 15 Oct", 1 "Thu Oct 15", 2 ISO "2026-10-15"
    bool week_starts_sunday = false;
};

// Miles only for the US measurement system: Qt reports the UK as "imperial" too, but UK
// hams (and the Raspberry Pi OS default en_GB locale) work in kilometres.
inline RegionDefaults region_defaults(const QLocale& locale) {
    RegionDefaults out;
    out.imperial = locale.measurementSystem() == QLocale::ImperialUSSystem;
    out.clock12h = locale.timeFormat(QLocale::ShortFormat).contains('a', Qt::CaseInsensitive);
    const QString date = locale.dateFormat(QLocale::ShortFormat).trimmed();
    out.date_order = date.startsWith('y', Qt::CaseInsensitive) ? 2 : date.startsWith('M') ? 1 : 0;
    out.week_starts_sunday = locale.firstDayOfWeek() == Qt::Sunday;
    return out;
}

} // namespace hamclock
