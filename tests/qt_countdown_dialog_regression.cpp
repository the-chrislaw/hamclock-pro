#include "../src/qt/countdown_dialog.hpp"
#include <QAccessible>
#include <QApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVariant>
#include <iostream>
#include <cmath>
#include <stdexcept>

namespace {
int checks = 0, failures = 0;
void check(bool okay, const char* label) {
    ++checks;
    if (!okay) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool same(const hamclock::CountdownServiceSnapshot& a, const hamclock::CountdownServiceSnapshot& b) {
    return a.configuration_epoch == b.configuration_epoch && a.model.phase == b.model.phase &&
        a.model.remaining.value == b.model.remaining.value && a.model.configured.value == b.model.configured.value &&
        a.model.run_id == b.model.run_id && a.model.expiry_pending == b.model.expiry_pending &&
        a.view.remaining_text == b.view.remaining_text && a.view.status == b.view.status &&
        a.dismiss_token.has_value() == b.dismiss_token.has_value();
}
double luminance(const QColor& color) {
    auto linear = [](double value) { return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4); };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}
double contrast(const QColor& a, const QColor& b) {
    const auto x = luminance(a), y = luminance(b);
    return (std::max(x, y) + 0.05) / (std::min(x, y) + 0.05);
}
} // namespace
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    using namespace hamclock;
    std::int64_t now = 0;
    int reads = 0;
    bool fail_clock = false;
    CountdownService service({kCountdownMinimumNanoseconds}, {0});
    auto clock = [&]() -> MonotonicNanoseconds {
        ++reads;
        if (fail_clock) throw std::runtime_error("<b>clock unavailable</b>");
        return {now};
    };
    bool threw = false;
    try { CountdownDialog invalid(service, {}); }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw && reads == 0 && service.snapshot().model.phase == CountdownPhase::Idle, "empty clock rejected without timer activation");
    CountdownDialog dialog(service, clock, "light");
    auto label = [&](const char* name) { return dialog.findChild<QLabel*>(name); };
    auto button = [&](const char* name) { return dialog.findChild<QPushButton*>(name); };
    auto* minutes = dialog.findChild<QSpinBox*>("countdownMinutes");
    auto* line = minutes->findChild<QLineEdit*>();
    auto accessible = [&](const char* name) {
        auto* interface = QAccessible::queryAccessibleInterface(label(name));
        return interface ? interface->text(QAccessible::Name) : QStringLiteral("missing");
    };
    check(reads == 0 && accessible("countdownRemaining").contains("00:01:00") &&
          accessible("countdownStatus").contains("Ready"), "constructor uses supplied configuration and current accessible values without clock sampling");
    dialog.show();
    app.processEvents();
    check(dialog.size().width() <= 800 && dialog.size().height() <= 480, "compact seven inch candidate layout");
    for (const char* name : {"countdownRun", "countdownPause", "countdownReset", "countdownDismiss", "countdownSet", "countdownExit",
                             "countdownDecrease", "countdownIncrease"}) {
        const auto* control = button(name);
        check(control && control->height() >= 44 && control->focusPolicy() == Qt::StrongFocus &&
              !control->accessibleName().isEmpty(), "named touch control has forty-four logical pixels and keyboard focus");
    }
    check(minutes->height() >= 44 && minutes->minimum() == 1 && minutes->maximum() == 10080,
          "minute draft editor uses explicit bounded range and touch height");
    check(minutes->buttonSymbols() == QAbstractSpinBox::NoButtons, "native half-height spin targets are replaced with touch buttons");
    reads = 0;
    button("countdownIncrease")->click();
    check(minutes->value() == 2 && reads == 0 && service.snapshot().model.configured.value == kCountdownMinimumNanoseconds,
          "plus changes draft only without clock or timer configuration");
    button("countdownDecrease")->click();
    check(minutes->value() == 1 && !button("countdownDecrease")->isEnabled(), "minus respects and disables minimum draft bound");
    minutes->setValue(10080);
    check(!button("countdownIncrease")->isEnabled(), "plus disables at maximum draft bound");
    check(label("countdownStatus")->textFormat() == Qt::PlainText &&
          label("countdownRemaining")->textFormat() == Qt::PlainText, "native errors and values cannot interpret rich text");
    minutes->setValue(2);
    now = 1;
    dialog.refreshNow();
    check(service.snapshot().model.configured.value == kCountdownMinimumNanoseconds && minutes->value() == 2,
          "draft changes and view ticks never configure timer implicitly");
    minutes->setFocus();
    QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    QApplication::sendEvent(minutes, &tab);
    check(QApplication::focusWidget() == button("countdownIncrease"), "Tab reaches plus draft control");
    QApplication::sendEvent(button("countdownIncrease"), &tab);
    check(QApplication::focusWidget() == button("countdownSet"), "next Tab reaches explicit configuration action");
    reads = 0;
    button("countdownRun")->click();
    check(reads == 1 && button("countdownRun")->text() == "Restart" && service.snapshot().model.phase == CountdownPhase::Running,
          "Start action samples clock once and relabels Restart");
    line->setText(QString());
    check(!dialog.perform(CountdownControl::SetAndReset), "invalid Set action reports error while timer keeps running");
    const auto running_error = accessible("countdownError");
    line->setText("2");
    for (int i = 1; i <= 3; ++i) {
        now = 1 + static_cast<std::int64_t>(i) * 1000000000;
        dialog.refreshNow();
        check(accessible("countdownError") == running_error && label("countdownStatus")->text() == "Running" &&
              service.snapshot().model.remaining.value == kCountdownMinimumNanoseconds - static_cast<std::int64_t>(i) * 1000000000,
              "running successful ticks keep action error while decrementing real countdown");
    }
    now = kCountdownMinimumNanoseconds;
    dialog.refreshNow();
    check(service.snapshot().model.remaining.value == 1 && label("countdownRemaining")->text() == "00:00:01",
          "native positive nanosecond display never announces premature zero");
    button("countdownPause")->click();
    now += 10000000000;
    dialog.refreshNow();
    check(button("countdownRun")->text() == "Resume" && service.snapshot().model.remaining.value == 1 &&
          accessible("countdownStatus").contains("Paused"), "Pause freezes time and displays accessible paused state");
    button("countdownRun")->click();
    ++now;
    dialog.refreshNow();
    check(button("countdownDismiss")->isEnabled() && service.snapshot().model.expiry_pending &&
          accessible("countdownRemaining").contains("00:00:00") && accessible("countdownStatus").contains("alert pending"),
          "expiry exposes zero pending state and captured dismissal control");
    auto rejected = [&](auto action, const char* name) {
        const auto before = service.snapshot();
        const auto last = service.last_observed();
        const auto remaining = label("countdownRemaining")->text();
        const auto draft = line->text();
        check(!action(), name);
        check(same(before, service.snapshot()) && last.value == service.last_observed().value &&
              remaining == label("countdownRemaining")->text() && draft == line->text(),
              "failure preserves service and displayed value and minute draft");
        check(accessible("countdownError").contains("Unable to update"), "failure exposed as named current accessibility error");
    };
    service.apply(CountdownCommand::Start, {now});
    now += kCountdownMinimumNanoseconds;
    service.advance({now});
    rejected([&] { return dialog.perform(CountdownControl::Dismiss); }, "old displayed token cannot clear newer external pending event");
    dialog.refreshNow();
    button("countdownDismiss")->click();
    check(!service.snapshot().model.expiry_pending && !button("countdownDismiss")->isEnabled() &&
          label("countdownStatus")->text() == "Time elapsed", "explicit current dismissal leaves expired zero without restart");
    reads = 0;
    rejected([&] { return dialog.perform(CountdownControl::Dismiss); }, "missing displayed token rejects before clock");
    check(reads == 0, "no authority means no clock read");
    rejected([&] { return dialog.perform(static_cast<CountdownControl>(999)); }, "unknown native control rejected");
    check(reads == 0, "invalid command does not sample clock");
    for (const QString& text : {QString(), QString("0"), QString("10081"), QString("1.5")}) {
        line->setText(text);
        reads = 0;
        rejected([&] { return dialog.perform(CountdownControl::SetAndReset); }, "invalid textual minute draft rejected without cached value apply");
        check(reads == 0, "invalid draft rejects before clock sampling");
    }
    const auto retained_error = accessible("countdownError");
    for (int i = 0; i < 3; ++i) {
        ++now;
        dialog.refreshNow();
        check(accessible("countdownError") == retained_error && label("countdownError")->isVisible() &&
              label("countdownStatus")->text() == "Time elapsed",
              "successful refresh ticks retain explicit action error while model status stays truthful");
    }
    line->setText("2");
    check(dialog.perform(CountdownControl::SetAndReset) && service.snapshot().configuration_epoch == 2 &&
          service.snapshot().model.configured.value == kCountdownMinimumNanoseconds * 2 &&
          service.snapshot().model.phase == CountdownPhase::Idle, "valid explicit draft commits new idle configuration");
    check(accessible("countdownError").isEmpty() && !label("countdownError")->isVisible(),
          "successful explicit action clears prior action error");
    minutes->setFocus();
    line->selectAll();
    QKeyEvent backspace(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
    QApplication::sendEvent(line, &backspace);
    check(!minutes->hasAcceptableInput(), "actual empty user edit remains intermediate");
    button("countdownSet")->setFocus();
    app.processEvents();
    reads = 0;
    rejected([&] { return dialog.perform(CountdownControl::SetAndReset); },
             "invalid user edit rejected even when native focus fixup restores cached number");
    check(reads == 0, "focus corrected invalid edit cannot silently apply cached timer value");
    minutes->setFocus();
    line->selectAll();
    QKeyEvent two(QEvent::KeyPress, Qt::Key_2, Qt::NoModifier, QStringLiteral("2"));
    QApplication::sendEvent(line, &two);
    check(accessible("countdownError").isEmpty(), "new valid user edit clears retained action error");
    check(dialog.perform(CountdownControl::SetAndReset), "new valid user edit clears invalid flag and permits explicit configuration");
    minutes->setValue(3);
    fail_clock = true;
    rejected([&] { return dialog.perform(CountdownControl::SetAndReset); }, "clock error does not configure or discard draft");
    check(accessible("countdownError").contains("<b>clock unavailable</b>") &&
          label("countdownError")->textFormat() == Qt::PlainText, "external error markup remains literal and accessible");
    fail_clock = false;
    const auto good = now;
    --now;
    rejected([&] { return dialog.refreshNow(); }, "reversed tick does not reset countdown");
    now = good;
    dialog.refreshNow();
    check(minutes->value() == 3, "same epoch successful tick preserves unsaved draft");
    check(accessible("countdownError").contains("<b>clock unavailable</b>") &&
          !accessible("countdownError").contains("Reversed"),
          "successful clock recovery clears tick error while preserving explicit action error");
    button("countdownRun")->click();
    button("countdownReset")->click();
    check(service.snapshot().model.phase == CountdownPhase::Idle && label("countdownStatus")->text() == "Ready",
          "explicit Reset returns ready without changing configuration");
    button("countdownRun")->click();
    auto* timer = dialog.findChild<QTimer*>(QString(), Qt::FindDirectChildrenOnly);
    check(timer && timer->interval() == 250 && timer->isActive(), "parented visible refresh timer has bounded cadence");
    dialog.reject();
    check(!timer->isActive() && service.snapshot().model.phase == CountdownPhase::Running, "closing stops view ticks only");
    now += 1000000000;
    dialog.show();
    check(service.snapshot().model.remaining.value == kCountdownMinimumNanoseconds * 2 - 1000000000,
          "reopen catches up caller service from injected time");
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&dialog, &escape);
    check(!dialog.isVisible() && !timer->isActive(), "Escape closes without countdown cancellation");
    {
        CountdownDialog recreated(service, clock, "dark");
        recreated.show();
        check(recreated.property("dashboardResolvedDark").toBool(), "dark uses existing approved theme helper");
    }
    check(service.snapshot().model.phase == CountdownPhase::Running, "dialog destruction preserves long-lived running service");
    CountdownDialog system(service, clock, "system");
    auto palette = app.palette();
    palette.setColor(QPalette::Window, QColor("#111111"));
    app.setPalette(palette); app.processEvents(); app.processEvents();
    check(system.property("dashboardResolvedDark").toBool(), "System follows live dark palette");
    palette.setColor(QPalette::Window, QColor("#fafafa"));
    app.setPalette(palette); app.processEvents(); app.processEvents();
    check(!system.property("dashboardResolvedDark").toBool(), "System follows live light palette");
    check(system.findChild<QSpinBox*>("countdownMinutes")->palette().color(QPalette::Base) == QColor("#fafaf7"),
          "hidden System input follows live light dialog palette");
    for (const std::string& mode : {std::string("light"), std::string("dark")}) {
        auto host = app.palette();
        host.setColor(QPalette::Window, QColor(mode == "light" ? "#111111" : "#fafafa"));
        app.setPalette(host);
        app.processEvents(); app.processEvents();
        CountdownDialog themed(service, clock, mode);
        themed.show(); app.processEvents();
        auto* field = themed.findChild<QSpinBox*>("countdownMinutes");
        const QColor base(mode == "light" ? "#fafaf7" : "#2b3236");
        const QColor ink(mode == "light" ? "#303a42" : "#e2e7e6");
        check(field->palette().color(QPalette::Base) == base && field->palette().color(QPalette::Text) == ink &&
              field->palette().color(QPalette::ButtonText) == ink &&
              field->findChild<QLineEdit*>()->palette().color(QPalette::Base) == base,
              "explicit field semantic colors override opposite host appearance");
        check(contrast(base, ink) >= 4.5 && contrast(base, themed.palette().color(QPalette::Highlight)) >= 3.0,
              "field text boundary and focus colors meet numerical contrast targets");
        const auto image = field->grab().toImage();
        check(image.pixelColor(image.width() / 2, image.height() - 6) == base,
              "rendered field interior uses intended semantic background");
        check(themed.size().width() <= 800 && themed.size().height() <= 480,
              "separate touch steppers retain compact candidate layout");
    }
    CountdownService fractional({kCountdownMinimumNanoseconds + 1}, {now});
    CountdownDialog fractional_dialog(fractional, clock);
    check(fractional_dialog.findChild<QSpinBox*>("countdownMinutes")->value() == 2 &&
          fractional.snapshot().model.configured.value == kCountdownMinimumNanoseconds + 1,
          "initial minute draft ceiling does not replace caller exact fractional duration");
    fractional_dialog.findChild<QSpinBox*>("countdownMinutes")->setValue(10080);
    check(fractional_dialog.perform(CountdownControl::SetAndReset) &&
          fractional.snapshot().model.configured.value == kCountdownMaximumNanoseconds,
          "maximum explicit editor duration multiplies safely to seven days");
    std::cout << checks << " native countdown checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
