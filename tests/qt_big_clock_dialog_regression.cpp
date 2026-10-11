#include "../src/qt/big_clock_dialog.hpp"
#include <QAbstractItemView>
#include <QAccessible>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPalette>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyleFactory>
#include <QStyleOptionButton>
#include <QTimer>
#include <QVariant>
#include <iostream>
#include <stdexcept>

namespace {
int checks = 0, failures = 0;
void check(bool condition, const char* label) {
    ++checks; if (!condition) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool same(const hamclock::BigClockPreferences& a, const hamclock::BigClockPreferences& b) {
    return a.face == b.face && a.zone == b.zone && a.hour_format == b.hour_format &&
        a.show_date == b.show_date && a.show_seconds == b.show_seconds && a.show_alarms == b.show_alarms &&
        a.show_countdown == b.show_countdown && a.show_de_weather == b.show_de_weather && a.show_space_weather == b.show_space_weather;
}
template<class Action> bool rejects(Action action) {
    try { action(); } catch (const std::invalid_argument&) { return true; } return false;
}
} // namespace
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    using namespace hamclock;
    const auto original_palette = QApplication::palette();
    BigClockPreferences prefs{BigClockFace::Digital, BigClockZone::Utc, BigClockHourFormat::TwentyFour,
        true, true, true, true, true, true};
    BigClockDigitalInput input{{BigClockCivilTime{1970,1,1,0,0,0}, std::nullopt}, std::nullopt, SyncStatus::Unknown};
    int calls = 0; bool fail = false, unknown_failure = false;
    auto snapshot = [&]() -> BigClockDigitalInput {
        ++calls;
        if (unknown_failure) throw 1;
        if (fail) throw std::runtime_error("<b>snapshot failed</b>");
        return input;
    };
    check(rejects([&] { BigClockDialog bad(prefs, {}, "light"); }) && calls == 0, "empty callable rejected before sampling");
    auto bad_preferences = prefs; bad_preferences.face = static_cast<BigClockFace>(255);
    check(rejects([&] { BigClockDialog bad(bad_preferences, snapshot, "light"); }) && calls == 0,
          "invalid constructor preferences never sample source");
    check(rejects([&] { BigClockDialog bad(prefs, snapshot, "bogus"); }) && calls == 0,
          "invalid constructor appearance never samples source");
    BigClockDialog dialog(prefs, snapshot, "light");
    auto label = [&](const char* name) { return dialog.findChild<QLabel*>(name); };
    auto button = [&](const char* name) { return dialog.findChild<QPushButton*>(name); };
    auto combo = [&](const char* name) { return dialog.findChild<QComboBox*>(name); };
    auto* timer = dialog.findChild<QTimer*>("bigClockRefreshTimer");
    check(calls == 1 && !timer->isActive() && !dialog.displayStale(), "constructor one snapshot, hidden timer idle");
    check(label("bigClockTime")->text() == "00:00:00" && label("bigClockStatus")->text().contains("Synchronization unknown"),
          "valid epoch time not conflated with unknown synchronization");
    check(QApplication::palette() == original_palette, "appearance never mutates application palette");
    dialog.show(); app.processEvents();
    check(timer->isActive() && timer->interval() == 1000 && calls == 2, "show samples once and starts one-second view timer");
    for (auto* item : dialog.findChildren<QLabel*>()) check(item->textFormat() == Qt::PlainText, "all labels reject rich-text interpretation");
    const auto* accessible = QAccessible::queryAccessibleInterface(label("bigClockTime"));
    check(accessible && accessible->text(QAccessible::Name).contains("00:00:00"), "actual AX time exposes dynamic value");
    button("bigClockOptions")->click(); app.processEvents();
    check(combo("bigClockFace")->hasFocus(), "Options focuses first named native control");
    for (auto* item : {static_cast<QWidget*>(button("bigClockOptions")), static_cast<QWidget*>(button("bigClockExit")),
        static_cast<QWidget*>(combo("bigClockFace")), static_cast<QWidget*>(combo("bigClockZoneOption")),
        static_cast<QWidget*>(combo("bigClockHourFormat")), static_cast<QWidget*>(dialog.findChild<QCheckBox*>("bigClockShowDate")),
        static_cast<QWidget*>(dialog.findChild<QCheckBox*>("bigClockShowSeconds"))}) {
        check(item->height() >= 44 && item->focusPolicy() == Qt::StrongFocus && !item->accessibleName().isEmpty(),
              "every option and action has named focus and touch-size target");
    }
    QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    QApplication::sendEvent(combo("bigClockFace"), &tab);
    check(combo("bigClockZoneOption")->hasFocus(), "Tab follows named options order");
    QApplication::sendEvent(combo("bigClockZoneOption"), &tab);
    check(combo("bigClockHourFormat")->hasFocus(), "Tab reaches hour format");
    QApplication::sendEvent(combo("bigClockHourFormat"), &tab);
    auto* date_control = dialog.findChild<QCheckBox*>("bigClockShowDate");
    auto* seconds_control = dialog.findChild<QCheckBox*>("bigClockShowSeconds");
    check(date_control->hasFocus(), "Tab reaches date checkbox");
    QApplication::sendEvent(date_control, &tab);
    check(seconds_control->hasFocus(), "Tab reaches seconds checkbox");
    QApplication::sendEvent(seconds_control, &tab);
    check(button("bigClockExit")->hasFocus(), "Tab reaches Exit after all five options");
    date_control->click();
    const auto click_edge = [&](QCheckBox* control, QPoint point) {
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(point), QPointF(control->mapToGlobal(point)),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(point), QPointF(control->mapToGlobal(point)),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(control, &press); QApplication::sendEvent(control, &release);
    };
    const auto checked_before = date_control->isChecked();
    click_edge(date_control, {date_control->width() - 2, 2});
    check(date_control->isChecked() != checked_before, "checkbox entire top-edge rectangle is an actual touch target");
    click_edge(date_control, {date_control->width() - 2, date_control->height() - 2});
    check(date_control->isChecked() == checked_before, "checkbox entire bottom-edge rectangle is an actual touch target");
    const auto* date_accessible = QAccessible::queryAccessibleInterface(date_control);
    check(!dialog.preferences().show_date && date_accessible && !date_accessible->state().checked,
          "native checkbox exposes committed option state to accessibility");
    date_control->click();
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&dialog, &escape);
    check(dialog.isVisible() && !dialog.findChild<QWidget*>("bigClockOptionsPanel")->isVisible() && button("bigClockOptions")->hasFocus(),
          "Escape closes options and restores focus before exit");

    calls = 0; input.preview = BigClockCivilTimes{BigClockCivilTime{2000,2,29,12,34,56}, std::nullopt};
    check(dialog.refreshNow() && calls == 1 && label("bigClockStatus")->text().contains("Preview") && label("bigClockTime")->text() == "12:34:56",
          "one injected preview snapshot and truthful time basis");
    auto de = prefs; de.zone = BigClockZone::De;
    calls = 0;
    check(dialog.setPreferences(de) && calls == 1 && dialog.preferences().zone == BigClockZone::De &&
          label("bigClockTime")->text() == "Time unavailable" && label("bigClockZone")->text() == "DE", "missing DE preserves selection without UTC fallback");
    auto analog = de; analog.face = BigClockFace::Analog;
    check(dialog.setPreferences(analog) && label("bigClockTime")->text() == "Analog unavailable" && same(dialog.preferences(), analog),
          "unsupported analog retained without digital substitution or other feature changes");
    check(label("bigClockUnavailableFeatures")->text().contains("unavailable"), "unsupported broad features truthful");
    input.preview.reset(); input.synchronization = SyncStatus::Synchronized;
    check(dialog.setPreferences(prefs), "restore valid explicit source preferences");
    const auto last_time = label("bigClockTime")->text(); const auto last_view = dialog.lastView();
    fail = true; calls = 0;
    check(!dialog.setPreferences(de) && calls == 1 && same(dialog.preferences(), prefs) &&
          dialog.lastView().time_text == last_view.time_text && label("bigClockTime")->text() == last_time,
          "failed preference snapshot preserves all committed prefs/view/time");
    check(dialog.displayStale() && label("bigClockStatus")->text().contains("stale") &&
          !label("bigClockStatus")->text().contains("Synchronized") && label("bigClockTime")->accessibleName().contains("stale"),
          "failed refresh masks old synchronized evidence with queryable stale status");
    const auto action_error = label("bigClockActionError")->text();
    check(action_error.contains("<b>") && label("bigClockActionError")->textFormat() == Qt::PlainText, "action error bounded plain text");
    fail = false; calls = 0;
    check(dialog.refreshNow() && calls == 1 && !dialog.displayStale() && label("bigClockSourceError")->text().isEmpty() &&
          label("bigClockActionError")->text() == action_error, "successful refresh recovers source but retains failed action error");
    QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection);
    check(label("bigClockActionError")->text() == action_error, "successful actual timer callback cannot hide failed option action");
    calls = 0;
    check(!dialog.setPreferences(bad_preferences) && calls == 0 && !dialog.displayStale() &&
          label("bigClockSourceError")->text().isEmpty(), "unknown preferences reject before source call without stale mutation");
    check(dialog.setPreferences(prefs) && label("bigClockActionError")->text().isEmpty(), "successful equal preference action clears persistent action error");
    button("bigClockOptions")->click(); app.processEvents();
    fail = true; calls = 0;
    combo("bigClockZoneOption")->setCurrentIndex(1);
    check(calls == 1 && combo("bigClockZoneOption")->currentIndex() == 0 && same(dialog.preferences(), prefs),
          "native option rollback blocks recursive source sampling and restores selection");
    fail = false; dialog.refreshNow();
    check(!label("bigClockActionError")->text().isEmpty(), "rollback error retained after source recovery");
    dialog.setPreferences(prefs);
    unknown_failure = true;
    check(!dialog.refreshNow() && dialog.displayStale() && label("bigClockSourceError")->text().contains("failed"), "non-standard snapshot exception visibly contained");
    unknown_failure = false;
    const auto old_day = input.actual.utc->day;
    input.actual.utc->day = 32;
    const auto before_constructor = calls;
    check(rejects([&] { BigClockDialog invalid(prefs, snapshot, "light"); }) && calls == before_constructor + 1 &&
          input.actual.utc->day == 32, "invalid initial snapshot propagates after exactly one call without source mutation");
    input.actual.utc->day = old_day;
    input.actual.utc->day = 32;
    check(!dialog.refreshNow() && label("bigClockTime")->text() == last_time, "invalid supplied civil snapshot does not replace old display");
    input.actual.utc->day = 1; dialog.refreshNow();
    dialog.hide(); calls = 0;
    QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection);
    check(!timer->isActive() && calls == 0, "hidden late timeout never samples source");
    dialog.show(); app.processEvents(); check(calls == 1, "reopen refreshes once without source mutation");

    for (const auto& style : QStyleFactory::keys()) {
        QApplication::setStyle(style);
        for (const auto& mode : {std::string("light"), std::string("dark"), std::string("system")}) {
            auto host = original_palette;
            host.setColor(QPalette::Window, mode == "dark" ? QColor("#ffffff") : QColor("#101010"));
            host.setColor(QPalette::Base, mode == "dark" ? QColor("#ffffff") : QColor("#101010"));
            QApplication::setPalette(host);
            BigClockDialog themed(prefs, snapshot, mode); themed.show(); app.processEvents();
            auto* canvas = themed.findChild<QScrollArea*>("bigClockScroll");
            const auto expected_canvas = themed.palette().color(QPalette::Window);
            const auto content_image = canvas->widget()->grab().toImage();
            const auto viewport_image = canvas->viewport()->grab().toImage();
            check(content_image.pixelColor(2, 2) == expected_canvas && viewport_image.pixelColor(2, 2) == expected_canvas,
                  "rendered content and viewport canvas match local theme under opposite host Base");
            themed.findChild<QPushButton*>("bigClockOptions")->click(); app.processEvents();
            for (auto* field : themed.findChildren<QComboBox*>()) {
                check(field->palette().color(QPalette::Base) == themed.palette().color(QPalette::Base) &&
                      field->palette().color(QPalette::Text) == themed.palette().color(QPalette::Text) &&
                      field->view()->palette().color(QPalette::Base) == themed.palette().color(QPalette::Base) &&
                      field->view()->palette().color(QPalette::HighlightedText) == themed.palette().color(QPalette::HighlightedText),
                      "every installed host style has coherent combo/editor/popup semantic colors");
                check(!field->styleSheet().contains("down-arrow") && field->height() >= 44, "original painted disclosure indicator avoids host/CSS arrow failures");
                const auto rendered = field->grab().toImage();
                int arrow_ink = 0;
                for (int y = field->height()/2 - 5; y <= field->height()/2 + 5; ++y)
                    for (int x = field->width() - 22; x < field->width() - 5; ++x)
                        if (rendered.pixelColor(x,y) == field->palette().color(QPalette::Text)) ++arrow_ink;
                check(arrow_ink >= 8, "rendered original disclosure triangle has visible contrasting ink");
                for (int row = 0; row < field->count(); ++row) check(field->view()->sizeHintForRow(row) >= 44, "popup choices have real minimum touch row height");
            }
            for (auto* checkbox : themed.findChildren<QCheckBox*>()) {
                QStyleOptionButton option; option.initFrom(checkbox); option.text = checkbox->text();
                const auto indicator = checkbox->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, checkbox);
                checkbox->setChecked(false); const auto unchecked = checkbox->grab().toImage();
                checkbox->setChecked(true); const auto checked = checkbox->grab().toImage();
                check(unchecked.pixelColor(indicator.center()) == themed.palette().color(QPalette::Window) &&
                      checked.pixelColor(indicator.left()+4, indicator.top()+3) == themed.palette().color(QPalette::Window),
                      "actual unchecked/checked indicator interiors use resolved local canvas, never host Base");
                int ink = 0, changed = 0;
                for (int y = indicator.top()+2; y < indicator.bottom()-1; ++y)
                    for (int x = indicator.left()+2; x < indicator.right()-1; ++x) {
                        if (checked.pixelColor(x,y) == themed.palette().color(QPalette::Text)) ++ink;
                        if (checked.pixelColor(x,y) != unchecked.pixelColor(x,y)) ++changed;
                    }
                check(ink >= 3 && changed >= 5, "rendered checked mark differs from unchecked with visible contrasting ink");
            }
            auto large_font = themed.font(); large_font.setPointSize(20); themed.setFont(large_font);
            for (const auto size : {QSize(800,480), QSize(1280,800)}) {
                themed.resize(size); app.processEvents();
                auto* scroll = themed.findChild<QScrollArea*>("bigClockScroll");
                check(scroll->viewport()->height() > 0 && scroll->horizontalScrollBar()->maximum() == 0,
                      "enlarged options reflow/scroll without horizontal screen clipping");
                for (auto* field : themed.findChildren<QComboBox*>()) check(field->height() >= field->sizeHint().height(), "enlarged native option text not height-clipped");
            }
            themed.hide();
        }
    }
    QPalette dark = original_palette; dark.setColor(QPalette::Window, QColor("#101010"));
    QPalette light = original_palette; light.setColor(QPalette::Window, QColor("#ffffff"));
    QApplication::setPalette(light);
    BigClockDialog system(prefs, snapshot, "system"); system.show(); app.processEvents();
    QApplication::setPalette(dark); app.processEvents(); app.processEvents();
    check(system.property("dashboardResolvedDark").toBool() && system.findChild<QComboBox*>("bigClockFace")->palette().color(QPalette::Base) == system.palette().color(QPalette::Base),
          "System palette changes update semantic combo colors without source preference rewrite");
    QApplication::setPalette(original_palette); app.processEvents(); app.processEvents();
    auto* disposable = new BigClockDialog(prefs, snapshot, "light");
    QPointer<QTimer> owned_timer = disposable->findChild<QTimer*>("bigClockRefreshTimer");
    disposable->show(); delete disposable; app.processEvents();
    check(owned_timer.isNull(), "destruction removes timer and its context-bound callbacks");

    // Source callback attempts nested transactions while a failed earlier
    // action remains visible. Nested attempts must neither sample nor overlay
    // the outer candidate, status/errors, controls or existing stale evidence.
    BigClockDialog* guarded_pointer = nullptr;
    int guarded_calls = 0;
    bool attempt_reentry = false, throw_after_reentry = false;
    auto guarded_source = [&]() -> BigClockDigitalInput {
        ++guarded_calls;
        if (attempt_reentry && guarded_pointer) {
            const auto before = guarded_pointer->lastView();
            const auto before_prefs = guarded_pointer->preferences();
            const bool before_stale = guarded_pointer->displayStale();
            const auto before_action = guarded_pointer->findChild<QLabel*>("bigClockActionError")->text();
            const auto before_source = guarded_pointer->findChild<QLabel*>("bigClockSourceError")->text();
            const auto before_status = guarded_pointer->findChild<QLabel*>("bigClockStatus")->text();
            auto nested = prefs; nested.zone = BigClockZone::De;
            check(!guarded_pointer->refreshNow() && !guarded_pointer->setPreferences(nested),
                  "nested snapshot transactions rejected without recursion");
            check(same(guarded_pointer->preferences(), before_prefs) &&
                  guarded_pointer->lastView().time_text == before.time_text &&
                  guarded_pointer->displayStale() == before_stale &&
                  guarded_pointer->findChild<QLabel*>("bigClockActionError")->text() == before_action &&
                  guarded_pointer->findChild<QLabel*>("bigClockSourceError")->text() == before_source &&
                  guarded_pointer->findChild<QLabel*>("bigClockStatus")->text() == before_status,
                  "reentrant rejection preserves complete outer-observable state and errors");
        }
        if (throw_after_reentry) throw std::bad_alloc();
        return input;
    };
    BigClockDialog guarded(prefs, guarded_source, "light"); guarded_pointer = &guarded;
    guarded.setPreferences(bad_preferences);
    const auto retained_action = guarded.findChild<QLabel*>("bigClockActionError")->text();
    attempt_reentry = true; guarded_calls = 0;
    check(guarded.refreshNow() && guarded_calls == 1 &&
          guarded.findChild<QLabel*>("bigClockActionError")->text() == retained_action,
          "outer refresh samples once and cannot lose persistent action error through reentry");
    auto proposed = prefs; proposed.show_seconds = false;
    throw_after_reentry = true; guarded_calls = 0;
    const auto prior = guarded.lastView();
    check(!guarded.setPreferences(proposed) && guarded_calls == 1 && same(guarded.preferences(), prefs) &&
          guarded.lastView().time_text == prior.time_text && guarded.displayStale(),
          "precommit allocation-style source failure preserves values and marks stale");
    throw_after_reentry = false; guarded_calls = 0;
    check(guarded.setPreferences(proposed) && guarded_calls == 1 && same(guarded.preferences(), proposed) &&
          !guarded.displayStale() && guarded.findChild<QLabel*>("bigClockActionError")->text().isEmpty(),
          "RAII in-flight guard recovers after throwing preparation and successful explicit action");
    std::cout << "Qt Big Clock dialog: " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
