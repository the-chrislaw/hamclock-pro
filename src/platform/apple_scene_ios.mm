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
//  6. Answers safeAreaInsets for Qt's window and views with the insets of a
//     hidden window created in the scene, and hides the status bar in landscape
//     on iPhone. Qt's window is created before the scene connects, so UIKit
//     never gives it an interface orientation: on iOS 26 it kept the portrait
//     insets (62 pt top, nothing at the sides) and the status bar in landscape,
//     and the Dynamic Island covered the left pane.
// Everything else (app state, keyboard, rotation) keeps working
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
NSString* const kQtViewClass = @"QUIView"; // every QWindow's UIView (QUIMetalView derives from it)
NSString* const kSceneConfigurationName = @"Default Configuration";

UIWindowScene* g_scene = nil;            // retained; the app has one scene
UIWindow* g_insetSource = nil;           // retained; hidden, created in g_scene for its current orientation
UIInterfaceOrientation g_insetOrientation = UIInterfaceOrientationUnknown;
NSHashTable<UIWindow*>* g_pending = nil; // Qt windows shown before the scene connected

using SetHiddenImp = void (*)(id, SEL, BOOL);
using SetScreenImp = void (*)(id, SEL, UIScreen*);
using InsetsImp = UIEdgeInsets (*)(id, SEL);
using BoolImp = BOOL (*)(id, SEL);
SetHiddenImp g_superSetHidden = nullptr;
SetScreenImp g_superSetScreen = nullptr;
InsetsImp g_superWindowInsets = nullptr;
InsetsImp g_superViewInsets = nullptr;
BoolImp g_qtStatusBarHidden = nullptr;
Class g_statusBarPatched = nil;

void releaseInsetSource() {
    [g_insetSource release];
    g_insetSource = nil;
    g_insetOrientation = UIInterfaceOrientationUnknown;
}

// The scene's safe-area insets for a window filling it, from a hidden window created with
// initWithWindowScene:. Qt's own window can't be used: it was created before the scene, its
// interface orientation stays unknown, and UIKit gives it the portrait insets whichever way the
// device is turned. A hidden window keeps the orientation it was created in, so it is created
// again whenever the scene's orientation has changed.
bool sceneInsets(UIEdgeInsets* insets) {
    if (!g_scene) return false;
    if (!g_insetSource || g_insetOrientation != g_scene.interfaceOrientation) {
        releaseInsetSource();
        g_insetSource = [[UIWindow alloc] initWithWindowScene:g_scene];
        g_insetSource.hidden = YES;
        g_insetSource.userInteractionEnabled = NO;
        g_insetOrientation = g_scene.interfaceOrientation;
    }
    const CGRect bounds = g_scene.coordinateSpace.bounds;
    if (!CGRectEqualToRect(g_insetSource.frame, bounds)) g_insetSource.frame = bounds;
    *insets = g_insetSource.safeAreaInsets;
    return true;
}

bool isLandscapePhone() {
    return g_scene && UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPhone &&
           UIInterfaceOrientationIsLandscape(g_scene.interfaceOrientation);
}

UIEdgeInsets qtWindowSafeAreaInsets(UIWindow* self, SEL cmd) {
    UIEdgeInsets insets;
    if (self.windowScene == g_scene && sceneInsets(&insets)) return insets;
    return g_superWindowInsets(self, cmd);
}

// A QWindow's view: the scene's insets less the part of them its frame already keeps clear of,
// which is what UIKit works out for a view in a window that has the right insets.
UIEdgeInsets qtViewSafeAreaInsets(UIView* self, SEL cmd) {
    UIWindow* window = self.window;
    UIEdgeInsets screen;
    if (!window || window.windowScene != g_scene || ![window isKindOfClass:NSClassFromString(kQtWindowClass)] ||
        !sceneInsets(&screen))
        return g_superViewInsets(self, cmd);
    const CGRect frame = [self convertRect:self.bounds toView:window];
    const CGRect whole = window.bounds;
    return UIEdgeInsetsMake(MAX(0, screen.top - CGRectGetMinY(frame)), MAX(0, screen.left - CGRectGetMinX(frame)),
                            MAX(0, screen.bottom - (CGRectGetMaxY(whole) - CGRectGetMaxY(frame))),
                            MAX(0, screen.right - (CGRectGetMaxX(whole) - CGRectGetMaxX(frame))));
}

// iPhone hides the status bar in landscape. Qt's view controller asks for it to show, which a
// window with an unknown orientation honours (iOS 26 drew it across the landscape screen).
BOOL qtPrefersStatusBarHidden(UIViewController* self, SEL cmd) {
    return isLandscapePhone() || (g_qtStatusBarHidden && g_qtStatusBarHidden(self, cmd));
}

void hideStatusBarInLandscape(UIViewController* controller) {
    Class cls = object_getClass(controller);
    if (!cls || g_statusBarPatched) return;
    Method method = class_getInstanceMethod(cls, @selector(prefersStatusBarHidden));
    if (!method) return;
    g_qtStatusBarHidden = reinterpret_cast<BoolImp>(class_replaceMethod(
        cls, @selector(prefersStatusBarHidden), reinterpret_cast<IMP>(qtPrefersStatusBarHidden), method_getTypeEncoding(method)));
    g_statusBarPatched = cls;
}

// After a rotation or a new scene: Qt re-reads the insets when its view controller lays out.
void refreshQtWindows() {
    if (!g_scene) return;
    Class qtWindow = NSClassFromString(kQtWindowClass);
    for (UIWindow* window in g_scene.windows) {
        if (![window isKindOfClass:qtWindow]) continue;
        UIViewController* controller = window.rootViewController;
        if (!controller) continue;
        hideStatusBarInLandscape(controller);
        [controller setNeedsStatusBarAppearanceUpdate];
        [window safeAreaInsetsDidChange];
        [controller.view setNeedsLayout];
        for (UIView* view in controller.view.subviews) [view safeAreaInsetsDidChange];
    }
}

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
    // Safe areas are not needed to launch: without these the app runs with UIKit's insets.
    Class qtView = NSClassFromString(kQtViewClass);
    g_superWindowInsets = reinterpret_cast<InsetsImp>(
        overrideOnSubclass(qtWindow, @selector(safeAreaInsets), reinterpret_cast<IMP>(qtWindowSafeAreaInsets)));
    if (qtView) {
        g_superViewInsets = reinterpret_cast<InsetsImp>(
            overrideOnSubclass(qtView, @selector(safeAreaInsets), reinterpret_cast<IMP>(qtViewSafeAreaInsets)));
    } else {
        NSLog(@"HamClock Pro: %@ not found; safe areas follow UIKit", kQtViewClass);
    }
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
    refreshQtWindows();
}

- (void)windowScene:(UIWindowScene*)windowScene
    didUpdateCoordinateSpace:(id<UICoordinateSpace>)previousCoordinateSpace
        interfaceOrientation:(UIInterfaceOrientation)previousInterfaceOrientation
             traitCollection:(UITraitCollection*)previousTraitCollection {
    (void)previousCoordinateSpace;
    (void)previousInterfaceOrientation;
    (void)previousTraitCollection;
    if (windowScene == g_scene) refreshQtWindows();
}

// UIKit may disconnect a background scene to reclaim memory and connect a new
// one later; Qt's windows outlive it, so park them until the next scene.
- (void)sceneDidDisconnect:(UIScene*)scene {
    if (scene != g_scene) return;
    Class qtWindow = NSClassFromString(kQtWindowClass);
    for (UIWindow* window in g_scene.windows) {
        if ([window isKindOfClass:qtWindow]) [g_pending addObject:window];
    }
    releaseInsetSource();
    [g_scene release];
    g_scene = nil;
}

@end
