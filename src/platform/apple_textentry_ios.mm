#include "apple_textentry.hpp"

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

namespace hamclock {

bool nativeHardwareKeyboardAttached() {
    return GCKeyboard.coalescedKeyboard != nil;
}

bool nativeIosAppOnMac() {
    return NSProcessInfo.processInfo.isiOSAppOnMac;
}

void nativeObserveKeyboard(std::function<void()> on_change) {
    auto cb = std::make_shared<std::function<void()>>(std::move(on_change));
    auto handler = ^(NSNotification*) { (*cb)(); };
    NSNotificationCenter* nc = NSNotificationCenter.defaultCenter;
    [nc addObserverForName:GCKeyboardDidConnectNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:handler];
    [nc addObserverForName:GCKeyboardDidDisconnectNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:handler];
}

} // namespace hamclock
