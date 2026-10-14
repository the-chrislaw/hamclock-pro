#include "apple_safearea.hpp"

#include <QWindow>
#import <UIKit/UIKit.h>

namespace hamclock {

QMarginsF nativeSafeAreaInsets(QWindow* window) {
    if (!window || !window->winId()) return {};
    UIView* view = reinterpret_cast<UIView*>(window->winId());
    const UIEdgeInsets in = view.safeAreaInsets; // points, same unit as Qt Quick
    return QMarginsF(in.left, in.top, in.right, in.bottom);
}

} // namespace hamclock
