#include "apple_lifecycle.hpp"

#include <QGuiApplication>

namespace hamclock {

AppLifecycle& AppLifecycle::instance() {
    static AppLifecycle lifecycle;
    return lifecycle;
}

void AppLifecycle::setState(Qt::ApplicationState state) {
    const bool now_suspended = state == Qt::ApplicationSuspended;
    if (now_suspended == suspended_) return;
    suspended_ = now_suspended;
    if (suspended_) emit suspended();
    else emit resumed();
}

void AppLifecycle::attachToApplication() {
#if defined(Q_OS_IOS)
    auto* app = qGuiApp;
    if (!app) return;
    setState(app->applicationState());
    connect(app, &QGuiApplication::applicationStateChanged, this, &AppLifecycle::setState,
            Qt::UniqueConnection);
#endif
}

} // namespace hamclock
