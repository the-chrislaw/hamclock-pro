#include "apple_passreminder.hpp"

namespace hamclock {

ReminderPlan planPassReminder(const QString& satellite, const QDateTime& aos_utc, const QDateTime& los_utc,
                              double max_el_deg, int lead_min, const QDateTime& now_utc) {
    ReminderPlan p;
    if (!aos_utc.isValid() || !los_utc.isValid() || !now_utc.isValid() || satellite.isEmpty()) return p;
    if (lead_min < 0) lead_min = 0;
    if (lead_min > 120) lead_min = 120;
    if (los_utc <= now_utc || los_utc <= aos_utc) return p;
    const QDateTime fire = aos_utc.addSecs(-static_cast<qint64>(lead_min) * 60);
    if (fire <= now_utc) return p;
    p.valid = true;
    p.fire_utc = fire.toUTC();
    p.id = QStringLiteral("pass.%1.%2").arg(satellite).arg(aos_utc.toSecsSinceEpoch());
    p.title = QStringLiteral("%1 pass in %2 min").arg(satellite).arg(lead_min);
    p.body = QStringLiteral("Max elevation %1°. Open HamClock Pro to find it in the sky.")
                 .arg(qRound(max_el_deg));
    return p;
}

#ifndef Q_OS_IOS
bool nativeSchedulePassReminder(const ReminderPlan&) { return false; }
void nativeCancelPassReminder(const QString&) {}
#endif

} // namespace hamclock
