#pragma once
#include "alarms.hpp"

#include <QObject>
#include <QTimer>

namespace hamclock {

// QML face of the alarms (session only, UTC). Ringing is visual: no sound is played yet.
class AlarmsModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList items READ items NOTIFY changed)
    Q_PROPERTY(bool ringing READ ringing NOTIFY ringingChanged)
    Q_PROPERTY(QString ringingText READ ringingText NOTIFY ringingChanged)
public:
    explicit AlarmsModel(QObject* parent = nullptr);
    QStringList items() const;
    bool ringing() const { return !ringing_.isEmpty(); }
    QString ringingText() const { return ringing_; }
    // Adds a daily (or one-time) alarm at hour:minute UTC; ignored when out of range or at 8 alarms.
    Q_INVOKABLE bool add(int hour, int minute, bool daily);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void setEnabled(int index, bool enabled);
    Q_INVOKABLE void dismiss();
    // Test hook: run one check between two instants.
    void check(const QDateTime& prev, const QDateTime& now);
signals:
    void changed();
    void ringingChanged();
private:
    QVector<Alarm> alarms_;
    QDateTime last_;
    QString ringing_;
    QTimer timer_;
};

}  // namespace hamclock
