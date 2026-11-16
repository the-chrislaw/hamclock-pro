// FocusVisibility: focus rings follow the keyboard (Pi keyboards, Mac, remotes sending keys)
// and hide after a tap or click, so a touched button or pane does not keep a ring (iPhone).
#include "../src/ui/focus_visibility.hpp"
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QTouchEvent>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}

void press_key(QObject* target, int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
    QKeyEvent event(QEvent::KeyPress, key, modifiers);
    QCoreApplication::sendEvent(target, &event);
}

void click(QObject* target) {
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(5, 5), QPointF(5, 5), QPointF(5, 5), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(target, &event);
}

void touch(QObject* target) {
    QEventPoint point(1, QEventPoint::State::Pressed, QPointF(5, 5), QPointF(5, 5));
    QTouchEvent event(QEvent::TouchBegin, QPointingDevice::primaryPointingDevice(), Qt::NoModifier, {point});
    QCoreApplication::sendEvent(target, &event);
}
} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    hamclock::FocusVisibility focus;
    focus.install(&app);
    QObject target;
    QObject theme;
    theme.setProperty("keyboardFocus", true);
    focus.mirrorTo(&theme, "keyboardFocus");

    check(!focus.keyboard(), "no ring before any key is pressed");
    check(!theme.property("keyboardFocus").toBool(), "the mirror starts with the current value");

    int changes = 0;
    QObject::connect(&focus, &hamclock::FocusVisibility::keyboardChanged, [&changes] { ++changes; });

    press_key(&target, Qt::Key_Tab);
    check(focus.keyboard(), "Tab shows focus rings");
    check(theme.property("keyboardFocus").toBool(), "the mirror follows a key press");
    press_key(&target, Qt::Key_Down);
    check(focus.keyboard() && changes == 1, "further keys change nothing");

    click(&target);
    check(!focus.keyboard(), "a click hides focus rings");
    check(!theme.property("keyboardFocus").toBool(), "the mirror follows a click");

    press_key(&target, Qt::Key_Right);
    touch(&target);
    check(!focus.keyboard(), "a touch hides focus rings");

    press_key(&target, Qt::Key_Shift, Qt::ShiftModifier);
    press_key(&target, Qt::Key_Meta, Qt::MetaModifier);
    check(!focus.keyboard(), "a modifier on its own does not show focus rings");

    press_key(&target, Qt::Key_Return);
    check(focus.keyboard(), "Return (remote select) shows focus rings");

    if (failures) return 1;
    std::cout << "focus visibility ok\n";
    return 0;
}
