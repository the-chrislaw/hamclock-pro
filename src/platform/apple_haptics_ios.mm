#include "apple_skyfinder.hpp"

#import <UIKit/UIKit.h>

namespace hamclock {

void nativeHapticTick() {
    static UIImpactFeedbackGenerator* gen = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
    [gen impactOccurred];
}

} // namespace hamclock
