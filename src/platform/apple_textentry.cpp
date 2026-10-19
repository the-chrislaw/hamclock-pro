#include "apple_textentry.hpp"

namespace hamclock {

QString TextEntryHint::message() const {
    if (!ios_app_on_mac_) return {};
    return tr("Typing does not work in the iPad app on a Mac. Please use the macOS version of HamClock Pro.");
}

void TextEntryHint::setState(bool hardware_keyboard, bool ios_app_on_mac) {
    if (hardware_keyboard == hardware_keyboard_ && ios_app_on_mac == ios_app_on_mac_) return;
    hardware_keyboard_ = hardware_keyboard;
    ios_app_on_mac_ = ios_app_on_mac;
    emit changed();
}

void TextEntryHint::refresh() {
#if defined(Q_OS_IOS)
    if (!observing_) {
        observing_ = true;
        nativeObserveKeyboard([this] { refresh(); });
    }
    setState(nativeHardwareKeyboardAttached(), nativeIosAppOnMac());
#endif
}

} // namespace hamclock
