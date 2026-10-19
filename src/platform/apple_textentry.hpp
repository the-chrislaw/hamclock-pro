#pragma once
// Text-entry environment on Apple platforms (#513). Qt for iOS feeds hardware
// key events through its UITextInput responder; in the "iPad app on a Mac"
// environment those keys never arrive, which leaves a dead text field. Key
// events cannot be used to detect that (none arrive), so the environment is
// detected up front and QML shows `message` next to text fields. Everywhere
// else the message is empty and layouts are unchanged.
#include <QObject>
#include <QString>
#include <functional>
#include <memory>

namespace hamclock {

class TextEntryHint : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool hardwareKeyboard READ hardwareKeyboard NOTIFY changed)
    Q_PROPERTY(bool iosAppOnMac READ iosAppOnMac NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
public:
    explicit TextEntryHint(QObject* parent = nullptr) : QObject(parent) {}

    bool hardwareKeyboard() const { return hardware_keyboard_; }
    bool iosAppOnMac() const { return ios_app_on_mac_; }
    // Empty when typing is expected to work.
    QString message() const;

    // Emits changed() only when a value differs.
    void setState(bool hardware_keyboard, bool ios_app_on_mac);
    // Re-reads the native state (no-op off iOS).
    void refresh();

signals:
    void changed();

private:
    bool hardware_keyboard_ = false;
    bool ios_app_on_mac_ = false;
    bool observing_ = false;
};

#if defined(Q_OS_IOS)
// Implemented in apple_textentry_ios.mm.
bool nativeHardwareKeyboardAttached();
bool nativeIosAppOnMac();
// Calls `on_change` (main thread) when a keyboard connects or disconnects.
void nativeObserveKeyboard(std::function<void()> on_change);
#endif

} // namespace hamclock
