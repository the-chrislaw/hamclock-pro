#include "timers_model.hpp"

namespace hamclock {

TimersModel::TimersModel(QObject* parent) : QObject(parent) {
    clock_.start();
    timer_.setInterval(100);
    connect(&timer_, &QTimer::timeout, this, [this] { if (stopwatch_.running() || countdown_.running()) emit tick(); });
    timer_.start();
}

void TimersModel::toggleStopwatch() {
    if (stopwatch_.running()) stopwatch_.stop(now()); else stopwatch_.start(now());
    emit tick();
}
void TimersModel::resetStopwatch() { stopwatch_.reset(); emit tick(); }
void TimersModel::toggleCountdown() {
    if (countdown_.running()) countdown_.stop(now());
    else {
        if (countdown_.expired(now())) countdown_.reset();
        countdown_.start(now());
    }
    emit tick();
}
void TimersModel::resetCountdown() { countdown_.reset(); emit tick(); }
void TimersModel::addCountdownMinutes(int minutes) {
    if (countdown_.running()) return;
    countdown_.set_duration_ms(countdown_.duration_ms() + qint64(minutes) * 60000);
    emit tick();
}

}  // namespace hamclock
