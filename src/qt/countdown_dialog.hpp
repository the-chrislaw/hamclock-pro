#pragma once

#include "../app/countdown_service.hpp"
#include <QDialog>
#include <functional>
#include <string>

class QLabel;
class QPushButton;
class QSpinBox;
class QTimer;

namespace hamclock {
enum class CountdownControl { Run, Pause, Reset, Dismiss, SetAndReset };

// Caller-owned service must outlive dialog; operations on QApplication thread,
// no concurrent writes. Dialog does not replace/destroy the service object.
// Clock is copied/nonempty, injected and sampled once for each valid action/tick;
// invalid enum/missing displayed dismissal token rejects before clock sampling.
// Construction reads snapshot without advancing/activating the service.
// View owns a250ms parented timer, active only while visible; close/Escape/hide/
// destruction stop view ticks only. Reopen catches up from injected monotonic
// instant; no reset/pause on exit. No persistence/suspend correction claims.
// Run: Idle Start, Running/Expired Restart via Start, Paused Resume. Pause uses
// Stop, Reset uses Reset. Dismiss uses ONLY token captured in displayed pending
// snapshot; never rereads latest event on click. Stale tokens reject atomically.
// Editable minutes1..10080, >=44logical input/button targets, explicit Set &
// reset action replaces configuration/cancels old run. Draft editing never
// mutates timer; initial draft ceil(currentconfigured/minute), with exact
// configured display retained. Same duration also resets, by service policy.
// Set rejects empty/intermediate/out-of-range edited text before clock read;
// invalid user editing remains flagged even if Qt later restores cached text
// on focus loss. A new valid user edit or successful epoch sync clears the flag.
// Native half-height spin arrows are hidden; named >=44logical minus/plus
// buttons edit ONLY bounded draft, never clock/service. These explicit user
// edits may restore a valid draft from the cached value and clear action error;
// disabled at min/max. Field palette follows dialog semantic colors/themes.
// Epoch change on successful refresh/action synchronizes draft to new service
// configuration; otherwise preserves unsaved edits. Step/minute/max gestures
// and pause controls are replacement policies, not documented original limits.
// Service/clock exceptions preserve service, displayed values/captured token/
// draft, except separate visible plain-text error label. Action errors persist
// across successful ticks, cleared by successful explicit action or new valid
// user text edit. Tick clock errors clear on successful tick/action; an action
// error and tick error can coexist. Phase/value labels keep truthful state.
// Qt rendering happens
// after committed service output; fatal allocation during Qt updates cannot be
// rolled back and is not covered by service transaction guarantees.
// Existing warm/slate System/Light/Dark helper, explicit phase/pending words,
// current dynamic accessibility names, visible focus; default560x360 compact.
// Session-only/no sound/GPIO/radio/network/settings/actual clock reads.
class CountdownDialog final : public QDialog {
public:
    using Clock = std::function<MonotonicNanoseconds()>;
    CountdownDialog(CountdownService&, Clock, const std::string& appearance = "system",
                    QWidget* parent = nullptr);
    bool refreshNow();
    bool perform(CountdownControl);
protected:
    bool event(QEvent*) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
private:
    void render(const CountdownServiceSnapshot&);
    void reportFailure(const QString&, bool action);
    void renderErrors();
    void styleMinuteEditor();
    CountdownService& service_;
    Clock clock_;
    std::optional<CountdownDismissToken> displayed_token_;
    std::uint64_t displayed_epoch_ = 0;
    bool invalid_draft_ = false;
    QString action_error_, clock_error_;
    QLabel *remaining_ = nullptr, *configured_ = nullptr, *status_ = nullptr;
    QLabel* error_ = nullptr;
    QPushButton *run_ = nullptr, *pause_ = nullptr, *dismiss_ = nullptr;
    QPushButton *decrease_ = nullptr, *increase_ = nullptr;
    QSpinBox* minutes_ = nullptr;
    QTimer* timer_ = nullptr;
};
} // namespace hamclock
