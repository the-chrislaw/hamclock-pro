#pragma once
// Backend of the on-screen keyboard (--touch-keyboard, touch-only Pi kiosks). Each tap
// is delivered to the focused item as a real key press and release, so text fields
// behave exactly as with a hardware keyboard: textEdited fires, validators and
// input masks apply, Return emits accepted.
#include <QObject>
#include <QString>

namespace hamclock {

class KeyInjector : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    // Types each character of `text` into `target`. False when there is no target.
    Q_INVOKABLE bool type(QObject* target, const QString& text) const;
    // Presses one key (Qt::Key_Backspace, Qt::Key_Return, Qt::Key_Left, ...).
    Q_INVOKABLE bool press(QObject* target, int key) const;
};

} // namespace hamclock
