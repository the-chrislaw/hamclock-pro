#pragma once
// App suspend/resume notifications. iPadOS freezes a backgrounded app and drops
// its sockets, so polling and long-lived connections (DX cluster, rigctld) must
// stop while suspended and reconnect on resume. On desktop the app is never
// suspended, so the singleton only ever reports "active" there.
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
    // Connects to QGuiApplication::applicationStateChanged; a no-op on desktop
    // platforms, where the app is never suspended.
    void attachToApplication();

signals:
    void suspended();
    void resumed();

private:
    AppLifecycle() = default;
    bool suspended_ = false;
};

} // namespace hamclock
