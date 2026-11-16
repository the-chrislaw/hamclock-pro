#include "focus_visibility.hpp"

#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>

namespace hamclock {

namespace {
bool isModifierOnly(int key) {
    switch (key) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
    case Qt::Key_CapsLock:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R:
        return true;
    default:
        return false;
    }
}
} // namespace

void FocusVisibility::install(QCoreApplication* app) {
    if (app) app->installEventFilter(this);
}

void FocusVisibility::mirrorTo(QObject* target, const char* property) {
    mirror_ = target;
    mirrorProperty_ = property;
    if (mirror_) mirror_->setProperty(mirrorProperty_.constData(), keyboard_);
}

bool FocusVisibility::eventFilter(QObject* watched, QEvent* event) {
    switch (event->type()) {
    case QEvent::KeyPress:
        if (!isModifierOnly(static_cast<QKeyEvent*>(event)->key())) setKeyboard(true);
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::TouchBegin:
    case QEvent::TabletPress:
        setKeyboard(false);
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

void FocusVisibility::setKeyboard(bool keyboard) {
    if (keyboard == keyboard_) return;
    keyboard_ = keyboard;
    if (mirror_) mirror_->setProperty(mirrorProperty_.constData(), keyboard_);
    emit keyboardChanged();
}

} // namespace hamclock
