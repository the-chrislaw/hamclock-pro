#pragma once
// Local pass reminders for "Find it in the sky" (#573 item 4). The planning
// maths is platform-free and tested on Linux; the iOS side hands the result to
// UNUserNotificationCenter (no server, no push).
#include <QDateTime>
#include <QString>

namespace hamclock {

struct ReminderPlan {
    bool valid = false;
    QString id;          // stable per satellite + AOS, so rescheduling replaces
    QDateTime fire_utc;  // when the notification should appear
    QString title;
    QString body;
};

// Plans a reminder `lead_min` minutes before AOS. Invalid when the pass has
// ended or the fire time is already past; when the pass is imminent but not
// over and the lead has elapsed, nothing is scheduled (the finder is live).
ReminderPlan planPassReminder(const QString& satellite, const QDateTime& aos_utc, const QDateTime& los_utc,
                              double max_el_deg, int lead_min, const QDateTime& now_utc);

// Schedules/cancels through UNUserNotificationCenter on iOS; returns false and
// does nothing elsewhere. Permission is requested on first schedule only.
bool nativeSchedulePassReminder(const ReminderPlan& plan);
void nativeCancelPassReminder(const QString& id);

} // namespace hamclock
