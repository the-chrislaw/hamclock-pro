#pragma once
// App suspend/resume notifications. iPadOS freezes a backgrounded app and drops
// its sockets, so polling and long-lived connections (DX cluster, rigctld) must
// stop while suspended and reconnect on resume. A sleeping Mac loses its network
// the same way (connections look open but are dead after wake), so system sleep
// counts as suspended too.
#include <QObject>
#include <Qt>

namespace hamclock {

class AppLifecycle : public QObject {
    Q_OBJECT
public:
    static AppLifecycle& instance();

    bool isSuspended() const { return suspended_; }
    // Feed an application state; the QGuiApplication hookup calls this. Only
    // Qt::ApplicationSuspended / Qt::ApplicationHidden-on-iOS style states are
    // treated as suspended; Inactive (e.g. a system dialog) keeps running.
    void setState(Qt::ApplicationState state);
    // Mac system sleep (true) and wake (false); the NSWorkspace hookup calls this.
    // The app is suspended while either it or the system is.
    void setSystemAsleep(bool asleep);
    // iOS: follows QGuiApplication::applicationStateChanged. macOS: follows system
    // sleep and wake. Elsewhere a no-op: the app is never suspended.
    void attachToApplication();

signals:
    void suspended();
    void resumed();

private:
    AppLifecycle() = default;
    void update();
    void attachToSystemSleep(); // macOS only (apple_lifecycle_mac.mm)
    bool app_suspended_ = false;
    bool system_asleep_ = false;
    bool suspended_ = false;
};

} // namespace hamclock
