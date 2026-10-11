#pragma once

#include "../app/stopwatch_presentation.hpp"
#include <QDialog>
#include <functional>
#include <string>

class QLabel;
class QPushButton;
class QTimer;

namespace hamclock {
enum class StopwatchControl { Run, Stop, Lap, Reset };

// Caller owns both model and display latch and must keep them alive longer
// than this dialog. Use only on the QApplication thread; no concurrent writes.
// Clock is injected, nonempty, copied and called once per tick/action; supplied
// monotonic instants may be negative but must not reverse model last_observed.
// Construction validates inputs and renders a local presentation copy without
// changing caller state. Dialog owns only widgets and a 50ms refresh timer.
// Closing stops view ticks only; reopening via show restarts ticks. No stop or
// reset on close/destruction. Counting catches up from injected monotonic time
// on next successful refresh/action, subject to the clock's suspend behavior.
// Run means Start when Idle, Resume when Stopped, advance when Running, then
// explicit Release. Stop means domain Stop then refresh (clears held display).
// Lap advances then Hold; repeated Lap keeps the first display latch, never
// appends domain history or consumes its lap cap. Reset means domain Reset then
// refresh. These are replacement policies, not full original parity claims.
// Invalid commands, clock exceptions/reversal/overflow, invalid snapshots and
// preparation exceptions preserve BOTH shared states. Build local candidates,
// prepare view/Qt text before nonthrowing swaps; report failure visibly with
// a named status. Qt rendering after commit is not a transactional guarantee
// against fatal process-wide allocation failure. No timer reset on errors.
// Targets >=44 logical pixels, visible keyboard focus/accessibility names and
// explicit Running / Display held / Stopped / Ready text (not color alone).
// 480x300 compact default; System/Light/Dark uses existing warm/slate helper.
class StopwatchDialog final : public QDialog {
public:
    using Clock = std::function<MonotonicNanoseconds()>;
    StopwatchDialog(StopwatchState&, StopwatchPresentation&, Clock,
                    const std::string& appearance = "system", QWidget* parent = nullptr);
    // Same action paths used by controls and timer, deterministic injected tests.
    // Return false and visible status on caught std::exception/unknown exception.
    bool refreshNow();
    bool perform(StopwatchControl);
protected:
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
private:
    bool update(bool action, StopwatchControl);
    void render(const StopwatchPresentationView&, const QString&, const QString&);
    void reportFailure(const QString&);
    StopwatchState& model_;
    StopwatchPresentation& presentation_;
    Clock clock_;
    QLabel *elapsed_ = nullptr, *status_ = nullptr, *actual_ = nullptr;
    QPushButton *run_ = nullptr, *stop_ = nullptr, *lap_ = nullptr;
    QTimer* timer_ = nullptr;
};
} // namespace hamclock
