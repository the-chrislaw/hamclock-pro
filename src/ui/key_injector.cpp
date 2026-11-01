#include "key_injector.hpp"

#include <QCoreApplication>
#include <QKeyEvent>

namespace hamclock {

namespace {
void send_key(QObject* target, int key, const QString& text) {
    QKeyEvent down(QEvent::KeyPress, key, Qt::NoModifier, text);
    QCoreApplication::sendEvent(target, &down);
    QKeyEvent up(QEvent::KeyRelease, key, Qt::NoModifier, text);
    QCoreApplication::sendEvent(target, &up);
}

// Qt key codes equal the upper-case code point for Latin-1 characters.
int key_for(QChar c) {
    return c == QLatin1Char('\n') ? Qt::Key_Return : static_cast<int>(c.toUpper().unicode());
}
} // namespace

bool KeyInjector::type(QObject* target, const QString& text) const {
    if (!target) return false;
    for (const QChar c : text) send_key(target, key_for(c), QString(c));
    return true;
}

bool KeyInjector::press(QObject* target, int key) const {
    if (!target) return false;
    send_key(target, key, key == Qt::Key_Return ? QStringLiteral("\r") : QString());
    return true;
}

} // namespace hamclock
