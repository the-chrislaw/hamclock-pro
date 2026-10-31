// UIScene life cycle for Qt for iOS older than 6.8 (docs/ios-uiscene.md).
//
// Apps linked against the iOS 27 SDK must adopt the scene-based life cycle or
// UIKit refuses to launch them ("UIScene life cycle is required"). Qt 6.8 and
// later adopt it themselves (QIOSApplicationDelegate implements
// application:configurationForConnectingSceneSession:options:), so CMake only
// compiles this file for older kits, such as the Qt 6.5 LTS kit used for local
// and CI simulator builds. It is never part of the Qt 6.8 TestFlight build.
//
// What it does, without touching Qt's own window and view controller set-up:
//  1. Gives Qt's app delegate the scene configuration method (only when it
//     doesn't already have one), naming HCPSceneDelegate as the scene delegate.
//  2. Attaches Qt's QUIWindow (created by QIOSScreen) to the connected window
//     scene when Qt first shows it, or when the scene connects if Qt showed it
//     first. A UIWindow without a scene is never displayed in a scene-based app.
//  3. Keeps Qt from detaching that window again: Qt 6.5 sets window.screen when
//     it shows the window, and setting a screen on a scene's window would move
//     it off the scene.
//  4. Answers Qt's UIScreen.qt_applicationFrame with the scene's bounds:
//     UIScreen.applicationFrame is empty under scenes, which made every Qt
//     window 0x0 (a black screen).
//  5. Applies the saved rotation lock again when the scene connects: main()
//     applies it before that, when there is no scene to lock.
// Everything else (app state, keyboard, rotation, safe areas) keeps working
// through the UIApplication notifications and view controller callbacks that
// UIKit still sends in a scene-based app.

#include "apple_orientation.hpp"

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

@interface HCPSceneDelegate : UIResponder <UIWindowSceneDelegate>
@end

namespace {

NSString* const kQtAppDelegateClass = @"QIOSApplicationDelegate";
NSString* const kQtWindowClass = @"QUIWindow";
NSString* const kSceneConfigurationName = @"Default Configuration";

UIWindowScene* g_scene = nil;            // retained; the app has one scene
NSHashTable<UIWindow*>* g_pending = nil; // Qt windows shown before the scene connected

using SetHiddenImp = void (*)(id, SEL, BOOL);
using SetScreenImp = void (*)(id, SEL, UIScreen*);
SetHiddenImp g_superSetHidden = nullptr;
SetScreenImp g_superSetScreen = nullptr;

void attachToScene(UIWindow* window) {
    if (!g_scene || window.windowScene == g_scene) return;
    window.windowScene = g_scene;
}

void qtWindowSetHidden(UIWindow* self, SEL cmd, BOOL hidden) {
    if (!hidden) {
        if (g_scene) {
            attachToScene(self);
        } else {
            [g_pending addObject:self];
        }
    }
    g_superSetHidden(self, cmd, hidden);
}

void qtWindowSetScreen(UIWindow* self, SEL cmd, UIScreen* screen) {
    // Same screen as the scene's: nothing to change, and calling through would
    // detach the window from its scene. A different screen (external display)
    // goes through unchanged.
    if (self.windowScene && self.windowScene.screen == screen) return;
    g_superSetScreen(self, cmd, screen);
}

// Qt 6.5 bounds a screen's available geometry by UIScreen.applicationFrame,
// which is empty in a scene-based app, so every window came out 0x0. Answer
// with the scene's bounds instead (what Qt uses on tvOS); Qt still subtracts
// the safe-area insets itself.
CGRect qtApplicationFrame(UIScreen* self, SEL) {
    if (g_scene && g_scene.screen == self) return g_scene.coordinateSpace.bounds;
    return self.bounds;
}

UISceneConfiguration* sceneConfiguration(id, SEL, UIApplication*, UISceneSession* session, UISceneConnectionOptions*) {
    UISceneConfiguration* config = [UISceneConfiguration configurationWithName:kSceneConfigurationName
                                                                   sessionRole:session.role];
    config.delegateClass = [HCPSceneDelegate class];
    return config;
}

// Overrides `selector` on `cls` (Qt's UIWindow subclass, which doesn't define it
// itself) and returns the UIWindow implementation to call through to.
IMP overrideOnSubclass(Class cls, SEL selector, IMP replacement) {
    Method inherited = class_getInstanceMethod(cls, selector);
    if (!inherited) return nullptr;
    IMP inheritedImp = method_getImplementation(inherited);
    if (!class_addMethod(cls, selector, replacement, method_getTypeEncoding(inherited))) {
        NSLog(@"HamClock Pro: %@ already overrides %@; scene shim not installed", NSStringFromClass(cls),
              NSStringFromSelector(selector));
        return nullptr;
    }
    return inheritedImp;
}

bool installQtOverrides() {
    // Qt's own UIScreen category method, so replacing it touches nothing else.
    SEL applicationFrame = NSSelectorFromString(@"qt_applicationFrame");
    Method frameMethod = class_getInstanceMethod([UIScreen class], applicationFrame);
    if (!frameMethod) {
        NSLog(@"HamClock Pro: UIScreen.qt_applicationFrame not found; scene shim not installed");
        return false;
    }
    Class qtWindow = NSClassFromString(kQtWindowClass);
    if (!qtWindow) {
        NSLog(@"HamClock Pro: %@ not found; scene shim not installed", kQtWindowClass);
        return false;
    }
    g_superSetHidden = reinterpret_cast<SetHiddenImp>(
        overrideOnSubclass(qtWindow, @selector(setHidden:), reinterpret_cast<IMP>(qtWindowSetHidden)));
    g_superSetScreen = reinterpret_cast<SetScreenImp>(
        overrideOnSubclass(qtWindow, @selector(setScreen:), reinterpret_cast<IMP>(qtWindowSetScreen)));
    if (!g_superSetHidden || !g_superSetScreen) return false;
    method_setImplementation(frameMethod, reinterpret_cast<IMP>(qtApplicationFrame));
    return true;
}

// Runs before main(), so before UIApplicationMain looks at the app delegate.
__attribute__((constructor)) void installSceneShim() {
    @autoreleasepool {
        Class delegate = NSClassFromString(kQtAppDelegateClass);
        if (!delegate) {
            NSLog(@"HamClock Pro: %@ not found; scene shim not installed", kQtAppDelegateClass);
            return;
        }
        SEL configure = @selector(application:configurationForConnectingSceneSession:options:);
        if ([delegate instancesRespondToSelector:configure]) return; // this Qt adopts scenes itself
        if (!installQtOverrides()) return;
        g_pending = [[NSHashTable weakObjectsHashTable] retain];
        class_addMethod(delegate, configure, reinterpret_cast<IMP>(sceneConfiguration), "@@:@@@");
    }
}

} // namespace

@implementation HCPSceneDelegate

- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
    (void)session;
    (void)options;
    if (g_scene || ![scene isKindOfClass:[UIWindowScene class]]) return;
    g_scene = [static_cast<UIWindowScene*>(scene) retain];
    for (UIWindow* window in g_pending.allObjects) {
        attachToScene(window);
        [window makeKeyAndVisible];
    }
    [g_pending removeAllObjects];
    hamclock::reapplyOrientationLock();
}

// UIKit may disconnect a background scene to reclaim memory and connect a new
// one later; Qt's windows outlive it, so park them until the next scene.
- (void)sceneDidDisconnect:(UIScene*)scene {
    if (scene != g_scene) return;
    Class qtWindow = NSClassFromString(kQtWindowClass);
    for (UIWindow* window in g_scene.windows) {
        if ([window isKindOfClass:qtWindow]) [g_pending addObject:window];
    }
    [g_scene release];
    g_scene = nil;
}

@end
