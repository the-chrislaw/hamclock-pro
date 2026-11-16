#pragma once
// Whether focus rings should show: after a key press (Pi and Mac keyboards, a remote that
// sends keys) until the next tap or click. A button or pane that took focus from a touch
// then shows no ring (iPhone), while keyboard and remote navigation keep theirs. QML reads
// it as Theme.keyboardFocus (see mirrorTo); text fields keep their own focus border.
#include <QObject>
#include <QPointer>
#include <QByteArray>

class QCoreApplication;

namespace hamclock {

class FocusVisibility : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool keyboard READ keyboard NOTIFY keyboardChanged)
public:
    using QObject::QObject;

    bool keyboard() const { return keyboard_; }
    // Watches every key, mouse, touch and tablet press the application delivers.
    void install(QCoreApplication* app);
    // Keeps `property` of `target` equal to keyboard() (the QML Theme singleton).
    void mirrorTo(QObject* target, const char* property);

signals:
    void keyboardChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setKeyboard(bool keyboard);

    bool keyboard_ = false;
    QPointer<QObject> mirror_;
    QByteArray mirrorProperty_;
};

} // namespace hamclock
