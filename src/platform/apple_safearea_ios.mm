#include "apple_safearea.hpp"

#include <QWindow>
#import <UIKit/UIKit.h>
#import <objc/runtime.h>

namespace hamclock {

namespace {
UIStatusBarStyle g_status_bar_style = UIStatusBarStyleLightContent;

// Qt's view controller picks its own status bar style (light text, made for the old
// black bands). Make that controller's class answer with the backdrop's style instead.
void follow_backdrop_status_bar(UIViewController* controller) {
    static Class patched = nil;
    Class cls = object_getClass(controller);
    if (!cls || cls == patched) return;
    IMP style = imp_implementationWithBlock(^UIStatusBarStyle(id) { return g_status_bar_style; });
    class_replaceMethod(cls, @selector(preferredStatusBarStyle), style, "q@:");
    patched = cls;
}
} // namespace

QMarginsF nativeSafeAreaInsets(QWindow* window) {
    if (!window || !window->winId()) return {};
    UIView* view = reinterpret_cast<UIView*>(window->winId());
    const UIEdgeInsets in = view.safeAreaInsets; // points, same unit as Qt Quick
    return QMarginsF(in.left, in.top, in.right, in.bottom);
}

void nativeApplyBackdrop(QWindow* window, const QColor& color, bool dark) {
    if (!window || !window->winId() || !color.isValid()) return;
    UIView* view = reinterpret_cast<UIView*>(window->winId());
    UIWindow* host = view.window;
    if (!host) return;
    UIColor* fill = [UIColor colorWithRed:color.redF() green:color.greenF() blue:color.blueF() alpha:1.0];
    // Qt keeps its view inside the safe area; everything around it shows the host
    // window and its root view (black by default).
    host.backgroundColor = fill;
    UIViewController* root = host.rootViewController;
    root.view.backgroundColor = fill;
    if (!root) return;
    g_status_bar_style = dark ? UIStatusBarStyleLightContent : UIStatusBarStyleDarkContent;
    follow_backdrop_status_bar(root);
    [root setNeedsStatusBarAppearanceUpdate];
}

} // namespace hamclock
