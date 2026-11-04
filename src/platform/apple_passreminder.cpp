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

PassReminderSync::PassReminderSync(QObject* parent)
    : PassReminderSync(nativeSchedulePassReminder, nativeCancelPassReminder, parent) {}
PassReminderSync::PassReminderSync(ScheduleFn schedule, CancelFn cancel, QObject* parent)
    : QObject(parent), schedule_(std::move(schedule)), cancel_(std::move(cancel)) {}

void PassReminderSync::sync(const QVariantList& rows, const QDateTime& now_utc) {
    QSet<QString> wanted;
    for (const QVariant& v : rows) {
        const QVariantMap r = v.toMap();
        if (!r.value("starred").toBool()) continue;
        const ReminderPlan p = planPassReminder(
            r.value("name").toString(), QDateTime::fromSecsSinceEpoch(r.value("aos").toLongLong(), Qt::UTC),
            QDateTime::fromSecsSinceEpoch(r.value("los").toLongLong(), Qt::UTC), r.value("maxEl").toDouble(),
            lead_min_, now_utc);
        if (!p.valid) continue;
        wanted.insert(p.id);
        if (!active_.contains(p.id) && schedule_ && schedule_(p)) active_.insert(p.id);
    }
    for (const QString& id : QSet<QString>(active_))
        if (!wanted.contains(id)) { if (cancel_) cancel_(id); active_.remove(id); }
}

void PassReminderSync::clear() {
    for (const QString& id : QSet<QString>(active_)) if (cancel_) cancel_(id);
    active_.clear();
}

void PassReminderSync::syncNow() {
    if (const QObject* model = sender()) sync(model->property("rows").toList(), QDateTime::currentDateTimeUtc());
}

#ifndef Q_OS_IOS
bool nativeSchedulePassReminder(const ReminderPlan&) { return false; }
void nativeCancelPassReminder(const QString&) {}
#endif

} // namespace hamclock
