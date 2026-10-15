#pragma once
// Stopwatch and countdown logic with injected time (milliseconds from any monotonic clock),
// so it is testable offline. The QML Timers page drives it through TimersModel.
#include <QString>
#include <algorithm>

namespace hamclock {

class Stopwatch {
public:
    void start(qint64 now_ms) { if (!running_) { running_ = true; started_ = now_ms; } }
    void stop(qint64 now_ms) { if (running_) { banked_ += now_ms - started_; running_ = false; } }
    void reset() { running_ = false; banked_ = 0; }
    bool running() const { return running_; }
    qint64 elapsed_ms(qint64 now_ms) const { return banked_ + (running_ ? std::max<qint64>(0, now_ms - started_) : 0); }
private:
    bool running_ = false;
    qint64 started_ = 0;
    qint64 banked_ = 0;
};

class Countdown {
public:
    void set_duration_ms(qint64 ms) { if (!running_) { duration_ = std::max<qint64>(0, ms); remaining_ = duration_; } }
    void start(qint64 now_ms) { if (!running_ && remaining_ > 0) { running_ = true; end_ = now_ms + remaining_; } }
    void stop(qint64 now_ms) { if (running_) { remaining_ = remaining_ms(now_ms); running_ = false; } }
    void reset() { running_ = false; remaining_ = duration_; }
    bool running() const { return running_; }
    qint64 duration_ms() const { return duration_; }
    qint64 remaining_ms(qint64 now_ms) const { return running_ ? std::max<qint64>(0, end_ - now_ms) : remaining_; }
    bool expired(qint64 now_ms) const { return duration_ > 0 && remaining_ms(now_ms) == 0; }
private:
    bool running_ = false;
    qint64 duration_ = 0;
    qint64 remaining_ = 0;
    qint64 end_ = 0;
};

// "H:MM:SS.t" for stopwatches (tenths), "H:MM:SS" for countdowns (rounded up so 0 means done).
inline QString format_stopwatch(qint64 ms) {
    ms = std::max<qint64>(0, ms);
    const qint64 s = ms / 1000;
    return QString("%1:%2:%3.%4").arg(s / 3600).arg(s / 60 % 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0')).arg(ms % 1000 / 100);
}
inline QString format_countdown(qint64 ms) {
    ms = std::max<qint64>(0, ms);
    const qint64 s = (ms + 999) / 1000;
    return QString("%1:%2:%3").arg(s / 3600).arg(s / 60 % 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0'));
}

}  // namespace hamclock
