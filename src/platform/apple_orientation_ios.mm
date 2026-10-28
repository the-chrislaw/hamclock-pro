#include "apple_orientation.hpp"

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

namespace {

UIInterfaceOrientationMask g_mask = UIInterfaceOrientationMaskAll;

// Installed on the root view controller's class; UIKit asks it on every rotation.
UIInterfaceOrientationMask supportedOrientations(id, SEL) { return g_mask; }

UIInterfaceOrientationMask maskFor(const QString& lock) {
    if (lock == QLatin1String("portrait")) return UIInterfaceOrientationMaskPortrait;
    if (lock == QLatin1String("landscape")) return UIInterfaceOrientationMaskLandscape;
    // iPhone apps don't rotate upside down (matches Info-ios.plist).
    return UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad ? UIInterfaceOrientationMaskAll
                                                                               : UIInterfaceOrientationMaskAllButUpsideDown;
}

} // namespace

namespace hamclock {

void applyOrientationLock(const QString& lock) {
    g_mask = maskFor(lock);
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:[UIWindowScene class]]) continue;
        UIWindowScene* windowScene = (UIWindowScene*)scene;
        for (UIWindow* window in windowScene.windows) {
            UIViewController* controller = window.rootViewController;
            if (!controller) continue;
            class_replaceMethod(object_getClass(controller), @selector(supportedInterfaceOrientations),
                                (IMP)supportedOrientations, "Q@:");
            if (@available(iOS 16.0, *)) {
                [controller setNeedsUpdateOfSupportedInterfaceOrientations];
            }
        }
        if (@available(iOS 16.0, *)) {
            auto* preferences = [[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:g_mask];
            [windowScene requestGeometryUpdateWithPreferences:preferences errorHandler:nil];
        }
    }
    if (@available(iOS 16.0, *)) return;
    [UIViewController attemptRotationToDeviceOrientation];
}

} // namespace hamclock
