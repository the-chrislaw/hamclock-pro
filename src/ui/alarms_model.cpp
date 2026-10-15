#include "alarms_model.hpp"

namespace hamclock {

AlarmsModel::AlarmsModel(QObject* parent) : QObject(parent) {
    last_ = QDateTime::currentDateTimeUtc();
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, [this] {
        const QDateTime now = QDateTime::currentDateTimeUtc();
        check(last_, now);
        last_ = now;
    });
    timer_.start();
}

QStringList AlarmsModel::items() const {
    QStringList out;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const Alarm& a : alarms_)
        out << QString("%1 · %2 · %3").arg(alarm_time_text(a.minute_of_day), a.daily ? "daily" : "once", alarm_in_text(a, now));
    return out;
}

bool AlarmsModel::add(int hour, int minute, bool daily) {
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || alarms_.size() >= 8) return false;
    Alarm a;
    a.minute_of_day = hour * 60 + minute;
    a.daily = daily;
    alarms_.push_back(a);
    emit changed();
    return true;
}

void AlarmsModel::remove(int index) {
    if (index < 0 || index >= alarms_.size()) return;
    alarms_.remove(index);
    emit changed();
}

void AlarmsModel::setEnabled(int index, bool enabled) {
    if (index < 0 || index >= alarms_.size()) return;
    alarms_[index].enabled = enabled;
    emit changed();
}

void AlarmsModel::dismiss() {
    if (ringing_.isEmpty()) return;
    ringing_.clear();
    emit ringingChanged();
}

void AlarmsModel::check(const QDateTime& prev, const QDateTime& now) {
    bool fired = false;
    for (Alarm& a : alarms_) {
        if (!alarm_due(a, prev, now)) continue;
        ringing_ = "Alarm " + alarm_time_text(a.minute_of_day);
        if (!a.daily) a.enabled = false;
        fired = true;
    }
    if (fired) { emit ringingChanged(); emit changed(); }
}

}  // namespace hamclock
