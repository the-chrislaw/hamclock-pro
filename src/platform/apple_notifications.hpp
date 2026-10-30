#pragma once
// Local notifications through UNUserNotificationCenter on iPhone, iPad and the
// Mac app (no server, no push, no entitlement). Elsewhere, and in unbundled
// Apple executables such as unit tests, supported() is false and every call
// completes at once with "nothing done". Completion callbacks always run on the
// main (Qt GUI) thread.
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <functional>
#include <vector>

namespace hamclock::notify {

struct LocalAlert {
    QString id;        // same id replaces a pending request
    QDateTime fire_utc;
    QString title;
    QString body;
    bool sound = true;
};

enum class Permission { Unsupported, NotDetermined, Denied, Granted };
QString permission_name(Permission p); // "unsupported", "unknown", "denied", "granted"

bool supported();
// Lets notifications show as banners while the app is in front (otherwise iOS hides them).
void install_foreground_presenter();
// Asks once (the system remembers the answer); reports the resulting state.
void request_permission(std::function<void(Permission)> done);
void query_permission(std::function<void(Permission)> done);
// Removes pending requests whose id starts with `prefix` and is not in `alerts`,
// then adds every alert. Reports how many of ours are pending afterwards.
void replace_alerts(const QString& prefix, const std::vector<LocalAlert>& alerts, std::function<void(int)> done);
// Removes every pending request whose id starts with `prefix`.
void cancel_alerts(const QString& prefix, std::function<void(int)> done);
// Pending request ids starting with `prefix` (the Settings count and the debug log).
void pending_ids(const QString& prefix, std::function<void(QStringList)> done);

} // namespace hamclock::notify
