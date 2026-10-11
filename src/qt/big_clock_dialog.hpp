#pragma once

#include "../app/big_clock_presentation.hpp"
#include <QDialog>
#include <functional>
#include <string>

class QCheckBox;
class QComboBox;
class QLabel;
class QKeyEvent;
class QPushButton;
class QTimer;
class QWidget;

namespace hamclock {

// Original read-only digital Big Clock dialog proposal; no original defaults,
// complete feature parity, source accuracy, Qt6.4/Pi/AX compliance certification.
// Snapshot supplies an owned immutable input by value; it may throw. It performs
// no clock/timezone/service mutation on behalf of this view. Caller must keep
// any captured source alive longer than the dialog and arrange its own guarded
// service lifetime. Dialog never retains references into a supplied snapshot.
// A snapshot callback must not destroy the dialog; the in-flight guard prevents
// nested transactions, not use-after-destruction or violated capture lifetime.
// Use exclusively on QApplication's thread, without concurrent source writes.
// No system clock reads, time conversion, network, provider acquisition, alarm
// scheduling, countdown control, persistence, audio or hardware interaction.
class BigClockDialog final : public QDialog {
public:
    using Snapshot = std::function<BigClockDigitalInput()>;

    // Explicit caller config; owns a preference value copy, not station settings.
    // Nonempty callable, known preference enums and appearance system/light/dark
    // required. Constructor samples exactly once after these input checks,
    // validates/fully prepares initial digital view and text before displaying.
    // Source/formatter/preparation exceptions during construction propagate;
    // a failed constructor yields no usable dialog. No caller state is changed.
    // Construction may allocate Qt widgets/copy the callable; this is not a
    // no-heap UI. Appearance uses existing per-dialog helper; no app-global
    // palette change. Snapshot/civil validation follows frozen formatter API.
    BigClockDialog(const BigClockPreferences&, Snapshot,
                   const std::string& appearance, QWidget* parent = nullptr);

    const BigClockPreferences& preferences() const noexcept;
    // Last SUCCESSFULLY prepared/committed view. Query stale separately: old
    // Synchronized evidence cannot be portrayed as fresh after a failed refresh.
    const BigClockDigitalView& lastView() const noexcept;
    bool displayStale() const noexcept;

    // One snapshot sample per refresh or valid preference attempt; actions and
    // timer use the same boundary. Preference validation precedes sampling.
    // Prepare candidate view plus bounded plain Qt text before committing local
    // prefs/view. Unknown preference rejects without invoking snapshot. Ordinary
    // source/formatter/preparation exceptions return false and preserve ALL last
    // successful prefs/view/time/date. Failed source/preparation refresh marks
    // displayed data stale, with a separate named PlainText source/tick error.
    // Next successful refresh restores freshness and clears that source error.
    // A failed explicit preference action ALSO has a separate named PlainText
    // action error, retained across successful timer/manual refreshes: recovery
    // must never conceal that the requested preference was not applied. Only
    // the next successful explicit setPreferences action clears the action error
    // (including a successful equal-value action). Opening/closing Options,
    // view visibility changes and source recovery do not clear it. An invalid
    // preference is rejected before sampling; it sets action error but does not
    // alter current display freshness or invent a source error.
    // Options selections return to prior prefs on failure with ALL control
    // signals blocked, so rollback cannot trigger reentrant updates/sampling.
    // Successful unsupported
    // Analog or missing DE desires are retained visibly, never silently reset.
    // Qt rendering after commit has no rollback guarantee against process-wide
    // allocation failure. Error rendering is best-effort under that failure.
    // Postcommit Qt rendering is outside the preparation catch: a rendering
    // exception propagates rather than returning false/claiming rollback of an
    // already committed preference. A per-dialog RAII guard covers preparation,
    // commit, error rollback and rendering; it clears on every return/exception.
    // Nested refreshNow/setPreferences returns false before validation/sampling,
    // preserving prefs/view/stale/errors/option controls without overlaying the
    // outer transaction. GUI-thread use alone does not forbid snapshot reentry.
    bool refreshNow();
    bool setPreferences(const BigClockPreferences&);

protected:
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    bool update(const BigClockPreferences&, bool action);
    void reportFailure(const QString&, bool action, bool source_failed);
    void showOptions();
    void applyOptionControls();

    BigClockPreferences preferences_;
    Snapshot snapshot_;
    BigClockDigitalView last_view_{};
    bool stale_ = false;
    bool updating_ = false;
    QLabel *time_ = nullptr, *date_ = nullptr, *zone_ = nullptr;
    QLabel *status_ = nullptr, *features_ = nullptr;
    QLabel *source_error_ = nullptr, *action_error_ = nullptr;
    QPushButton *options_button_ = nullptr, *exit_button_ = nullptr;
    QWidget* options_panel_ = nullptr;
    QComboBox *face_ = nullptr, *zone_option_ = nullptr, *hour_format_ = nullptr;
    QCheckBox *show_date_ = nullptr, *show_seconds_ = nullptr;
    QTimer* timer_ = nullptr;
};

// Layout/interaction contract for later implementation gate:
// - Default 800x480 with standard layouts and scalable system font. Large time
//   label plus zone/date/status; no fixed screen dependency or clipped enlarged
//   text. Native Options and Exit controls >=44 logical pixels, explicit focus
//   and names. Options panel has face/zone/hour format/date/seconds controls with
//   native labels/buddies, tab order and selected-value metadata. Other four
//   weather/alarm/countdown desires are copied untouched and clearly unavailable
//   in this view. No fabricated live readings, alarm times or timer values.
//   EVERY option control has >=44 logical pixel target height and explicit
//   keyboard focus. Cross-host combo popup/editor Base/Text/selection/arrow and
//   checkbox checked/focus states must remain legible in Light/Dark/System;
//   native geometry alone does not establish contrast or enlarged-font fit.
// - Visible Options toggles the panel; Escape closes it first/restores Options
//   focus, otherwise exits. Exit/close never modifies a service. Background-click
//   menu compatibility is excluded from this small slice, not claimed complete.
// - Named time/zone/date and meaningful status expose dynamic values: Actual or
//   Preview, Unknown/Synchronized/Unsynchronized evidence, missing selected-zone
//   time or unsupported Analog. Do not rely on color. Queryable values are not
//   incessantly announced every tick. PlainText throughout, bounded error text.
// - QObject-parented/context-bound 1s QTimer, active only while visible. Show
//   refreshes once/restarts ticks; hide/close stops ticks; a late timeout checks
//   visibility before sampling. Destruction disconnects callbacks through Qt
//   ownership. No source change on close, view ticks are not time accuracy.
// - System/Light/Dark via existing helper; no new theme setting or dependencies.
//   Headless tests, actual computer-control QA, Qt6.4 and Pi/font/touch/AX checks
//   remain separate gates before claims or production dashboard integration.

} // namespace hamclock
