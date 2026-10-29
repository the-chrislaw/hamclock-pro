#include "apple_orientation.hpp"

namespace hamclock {

#ifndef Q_OS_IOS
void applyOrientationLock(const QString&) {}
#endif

} // namespace hamclock
