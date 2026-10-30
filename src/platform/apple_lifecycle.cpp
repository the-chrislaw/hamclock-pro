#include "apple_lifecycle.hpp"

#include <QGuiApplication>

namespace hamclock {

AppLifecycle& AppLifecycle::instance() {
    static AppLifecycle lifecycle;
    return lifecycle;
}

void AppLifecycle::setState(Qt::ApplicationState state) {
    app_suspended_ = state == Qt::ApplicationSuspended;
    update();
}

void AppLifecycle::setSystemAsleep(bool asleep) {
    system_asleep_ = asleep;
    update();
}

void AppLifecycle::update() {
    const bool now_suspended = app_suspended_ || system_asleep_;
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
#elif defined(Q_OS_MACOS)
    attachToSystemSleep();
#endif
}

#if !defined(Q_OS_MACOS)
void AppLifecycle::attachToSystemSleep() {}
#endif

} // namespace hamclock
