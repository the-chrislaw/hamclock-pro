#pragma once
#include "timers.hpp"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace hamclock {

// QML face of the stopwatch and countdown. State lives for the session only.
class TimersModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString stopwatchText READ stopwatchText NOTIFY tick)
    Q_PROPERTY(bool stopwatchRunning READ stopwatchRunning NOTIFY tick)
    Q_PROPERTY(QString countdownText READ countdownText NOTIFY tick)
    Q_PROPERTY(bool countdownRunning READ countdownRunning NOTIFY tick)
    Q_PROPERTY(bool countdownExpired READ countdownExpired NOTIFY tick)
public:
    explicit TimersModel(QObject* parent = nullptr);
    QString stopwatchText() const { return format_stopwatch(stopwatch_.elapsed_ms(now())); }
    bool stopwatchRunning() const { return stopwatch_.running(); }
    QString countdownText() const { return format_countdown(countdown_.remaining_ms(now())); }
    bool countdownRunning() const { return countdown_.running(); }
    bool countdownExpired() const { return countdown_.expired(now()); }
    Q_INVOKABLE void toggleStopwatch();
    Q_INVOKABLE void resetStopwatch();
    Q_INVOKABLE void toggleCountdown();
    Q_INVOKABLE void resetCountdown();
    // Adds minutes to the countdown while it is stopped (negative subtracts, floor 0).
    Q_INVOKABLE void addCountdownMinutes(int minutes);
signals:
    void tick();
private:
    qint64 now() const { return clock_.elapsed(); }
    QElapsedTimer clock_;
    QTimer timer_;
    Stopwatch stopwatch_;
    Countdown countdown_;
};

}  // namespace hamclock
