#pragma once
// In-app screen rotation lock for iPhone/iPad. Qt's iOS view controller lets the
// device rotate freely and has no lock of its own, so the native side answers
// UIKit's supportedInterfaceOrientations from the saved setting and asks the
// scene to rotate to it. A no-op everywhere else.
#include <QString>

namespace hamclock {

// "portrait" or "landscape" lock the app to that orientation; anything else
// ("auto") follows the device. Safe to call before or after the window shows.
void applyOrientationLock(const QString& lock);

} // namespace hamclock
