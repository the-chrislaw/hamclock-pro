#include "../src/qt/stopwatch_dialog.hpp"
#include <QApplication>
#include <QAccessible>
#include <QKeyEvent>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QTimer>
#include <QVariant>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
int checks = 0, failures = 0;
void check(bool condition, const char* label) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool same(const hamclock::StopwatchPresentationView& a,
          const hamclock::StopwatchPresentationView& b) {
    return a.phase == b.phase && a.label == b.label && a.held == b.held &&
        a.actual_elapsed.value == b.actual_elapsed.value &&
        a.displayed_elapsed.value == b.displayed_elapsed.value &&
        a.formatted == b.formatted && a.lap_count == b.lap_count;
}
bool same(const hamclock::StopwatchSnapshot& a, const hamclock::StopwatchSnapshot& b) {
    if (a.phase != b.phase || a.elapsed.value != b.elapsed.value || a.lap_count != b.lap_count) return false;
    for (std::size_t i = 0; i < a.laps.size(); ++i) {
        if (a.laps[i].number != b.laps[i].number || a.laps[i].total.value != b.laps[i].total.value ||
            a.laps[i].interval.value != b.laps[i].interval.value) return false;
    }
    return true;
}
} // namespace
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    using namespace hamclock;
    std::int64_t now = -100;
    int reads = 0;
    bool clock_failure = false;
    StopwatchState model({now});
    StopwatchPresentation presentation;
    auto clock = [&]() -> MonotonicNanoseconds {
        ++reads;
        if (clock_failure) throw std::runtime_error("<b>clock failed</b>");
        return {now};
    };
    bool threw = false;
    try { StopwatchDialog invalid(model, presentation, {}); }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw && model.last_observed().value == -100, "empty clock rejected without changing model");
    StopwatchDialog dialog(model, presentation, clock, "light");
    auto label = [&](const char* name) { return dialog.findChild<QLabel*>(name); };
    auto button = [&](const char* name) { return dialog.findChild<QPushButton*>(name); };
    auto accessible = [&](const char* name) {
        const auto* interface = QAccessible::queryAccessibleInterface(label(name));
        return interface ? interface->text(QAccessible::Name) : QStringLiteral("missing interface");
    };
    check(accessible("stopwatchElapsed") == "Displayed stopwatch elapsed: 00:00:00.00" &&
          accessible("stopwatchStatus") == "Stopwatch status: Ready", "initial accessible values expose displayed duration and state");
    check(reads == 0 && model.snapshot().phase == StopwatchPhase::Idle,
          "construction renders without clock read or model mutation");
    dialog.show();
    app.processEvents();
    check(dialog.size().width() <= 800 && dialog.size().height() <= 480,
          "compact seven inch candidate layout");
    for (const char* name : {"stopwatchRun", "stopwatchStop", "stopwatchLap", "stopwatchReset", "stopwatchExit"}) {
        const auto* control = button(name);
        check(control && !control->accessibleName().isEmpty() && control->height() >= 44 &&
              control->focusPolicy() == Qt::StrongFocus, "named focusable touch target");
    }
    check(label("stopwatchStatus")->textFormat() == Qt::PlainText &&
          label("stopwatchElapsed")->textFormat() == Qt::PlainText, "status cannot interpret markup");
    check(!button("stopwatchLap")->isEnabled(), "idle lap disabled");
    button("stopwatchRun")->setFocus();
    app.processEvents();
    QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    QApplication::sendEvent(button("stopwatchRun"), &tab);
    check(QApplication::focusWidget() == button("stopwatchReset"), "Tab skips disabled controls to Reset");
    reads = 0;
    button("stopwatchRun")->click();
    check(reads == 1 && model.snapshot().phase == StopwatchPhase::Running, "native Run starts once with one read");
    now = 1250000000 - 100;
    check(dialog.refreshNow() && label("stopwatchElapsed")->text() == "00:00:01.25", "deterministic elapsed tick");
    check(accessible("stopwatchElapsed").contains("00:00:01.25") &&
          accessible("stopwatchStatus").contains("Running"), "tick refreshes actual QAccessible duration and status");
    button("stopwatchLap")->click();
    now = 2250000000 - 100;
    dialog.refreshNow();
    check(presentation.view().held && model.snapshot().elapsed.value == 2250000000 &&
          label("stopwatchElapsed")->text() == "00:00:01.25" &&
          label("stopwatchActual")->text().contains("02.25") &&
          label("stopwatchStatus")->text().contains("counting continues"), "Lap holds view while actual advances");
    check(accessible("stopwatchElapsed").contains("01.25") &&
          accessible("stopwatchActual") == "Actual elapsed: 00:00:02.25" &&
          accessible("stopwatchStatus").contains("Display held"), "held accessibility exposes both displayed and actual elapsed");
    for (int i = 0; i < 70; ++i) check(dialog.perform(StopwatchControl::Lap), "repeated Lap never consumes history capacity");
    check(model.snapshot().lap_count == 0, "UI Lap is display hold without history append");
    button("stopwatchRun")->click();
    check(!presentation.view().held && label("stopwatchElapsed")->text() == "00:00:02.25", "Run explicitly releases display");
    check(accessible("stopwatchActual").isEmpty() && label("stopwatchActual")->text().isEmpty(),
          "release clears stale held secondary accessible value");
    now += 750000000;
    button("stopwatchStop")->click();
    now += 10000000000;
    dialog.refreshNow();
    check(model.snapshot().elapsed.value == 3000000000 && label("stopwatchStatus")->text() == "Stopped",
          "Stop excludes later stopped interval");
    check(accessible("stopwatchStatus") == "Stopwatch status: Stopped" &&
          accessible("stopwatchElapsed").contains("00:00:03.00"), "Stop exposes frozen duration and stopped accessibility state");
    button("stopwatchRun")->click();
    now += 1000000000;
    dialog.refreshNow();
    check(model.snapshot().elapsed.value == 4000000000, "Run resumes excluding pause");
    auto rejected = [&](auto action, const char* name) {
        const auto state = model.snapshot();
        const auto last = model.last_observed();
        const auto view = presentation.view();
        check(!action(), name);
        check(same(state, model.snapshot()) && last.value == model.last_observed().value &&
              same(view, presentation.view()), "failure preserves both shared states including clock/laps/latch");
        check(label("stopwatchStatus")->text().startsWith("Unable to update"), "failure visibly reported");
    };
    const auto good = now;
    --now;
    rejected([&] { return dialog.refreshNow(); }, "reversed clock rejected");
    now = good;
    dialog.perform(StopwatchControl::Lap);
    clock_failure = true;
    rejected([&] { return dialog.perform(StopwatchControl::Reset); }, "clock exception does not reset");
    check(label("stopwatchStatus")->text().contains("<b>clock failed</b>"), "external error displayed as literal text");
    check(accessible("stopwatchStatus").contains("Unable to update") &&
          accessible("stopwatchStatus").contains("<b>clock failed</b>"), "error updates actual accessibility name with literal failure");
    clock_failure = false;
    reads = 0;
    rejected([&] { return dialog.perform(static_cast<StopwatchControl>(999)); }, "invalid control rejected");
    check(reads == 0, "invalid command rejected before clock called");
    button("stopwatchReset")->click();
    check(model.snapshot().phase == StopwatchPhase::Idle && !presentation.view().held &&
          label("stopwatchElapsed")->text() == "00:00:00.00", "Reset explicitly clears both state displays");
    check(accessible("stopwatchStatus") == "Stopwatch status: Ready" &&
          accessible("stopwatchElapsed").contains("00:00:00.00") &&
          accessible("stopwatchActual").isEmpty(), "Reset clears held accessibility and exposes reset value");
    button("stopwatchRun")->click();
    auto* timer = dialog.findChild<QTimer*>(QString(), Qt::FindDirectChildrenOnly);
    check(timer && timer->isActive(), "visible view timer running");
    dialog.reject();
    check(timer && !timer->isActive() && model.snapshot().phase == StopwatchPhase::Running,
          "close stops view timer only");
    now += 2000000000;
    dialog.show();
    check(model.snapshot().elapsed.value == 2000000000 && timer->isActive(), "reopen catches up shared service");
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&dialog, &escape);
    check(!dialog.isVisible() && !timer->isActive(), "Escape closes view without reset");
    now += 1000000000;
    {
        StopwatchDialog reopened(model, presentation, clock, "dark");
        reopened.show();
        check(reopened.property("dashboardResolvedDark").toBool() && model.snapshot().elapsed.value == 3000000000,
              "new dialog reuses long lived caller state and dark helper");
        reopened.hide();
    }
    check(model.snapshot().phase == StopwatchPhase::Running, "destruction never stops caller service");
    StopwatchDialog system(model, presentation, clock, "system");
    auto palette = app.palette();
    palette.setColor(QPalette::Window, QColor("#fafafa"));
    app.setPalette(palette);
    app.processEvents();
    app.processEvents();
    check(!system.property("dashboardResolvedDark").toBool(), "System resolves light application palette");
    palette.setColor(QPalette::Window, QColor("#101010"));
    app.setPalette(palette);
    app.processEvents();
    app.processEvents();
    check(system.property("dashboardResolvedDark").toBool(), "System follows live dark application palette");
    StopwatchState overflow({std::numeric_limits<std::int64_t>::min()});
    overflow.apply(StopwatchCommand::Start, {std::numeric_limits<std::int64_t>::min()});
    StopwatchPresentation overflow_view;
    StopwatchDialog max_dialog(overflow, overflow_view, [] { return MonotonicNanoseconds{std::numeric_limits<std::int64_t>::max()}; });
    check(!max_dialog.refreshNow() && overflow.snapshot().elapsed.value == 0 &&
          overflow.last_observed().value == std::numeric_limits<std::int64_t>::min(), "overflow tick preserves shared model");
    check(max_dialog.perform(StopwatchControl::Reset) && overflow.snapshot().phase == StopwatchPhase::Idle,
          "explicit reset recovers overflow without discarded interval calculation");
    std::cout << checks << " native stopwatch checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
