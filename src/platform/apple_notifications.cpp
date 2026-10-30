// Non-Apple builds: no system notification centre; the in-app banner does the work.
#include "apple_notifications.hpp"

namespace hamclock::notify {

QString permission_name(Permission p) {
    switch (p) {
    case Permission::Granted: return QStringLiteral("granted");
    case Permission::Denied: return QStringLiteral("denied");
    case Permission::NotDetermined: return QStringLiteral("unknown");
    case Permission::Unsupported: break;
    }
    return QStringLiteral("unsupported");
}

#ifndef Q_OS_DARWIN
bool supported() { return false; }
void install_foreground_presenter() {}
void request_permission(std::function<void(Permission)> done) { if (done) done(Permission::Unsupported); }
void query_permission(std::function<void(Permission)> done) { if (done) done(Permission::Unsupported); }
void replace_alerts(const QString&, const std::vector<LocalAlert>&, std::function<void(int)> done) { if (done) done(0); }
void cancel_alerts(const QString&, std::function<void(int)> done) { if (done) done(0); }
void pending_ids(const QString&, std::function<void(QStringList)> done) { if (done) done({}); }
#endif

} // namespace hamclock::notify
