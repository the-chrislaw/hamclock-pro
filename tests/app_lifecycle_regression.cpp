// Suspend/resume: PolledSource stops polling while the app is suspended and
// refreshes on resume (immediately when due). Offline; network is faked.
#include "../src/net/polled_source.hpp"
#include "../src/platform/apple_lifecycle.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
void spin(QCoreApplication& app) { for (int i = 0; i < 10; ++i) app.processEvents(); }
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("hamclock-pro-lifecycle-test");
    QTemporaryDir cache;
    qputenv("XDG_CACHE_HOME", cache.path().toUtf8());
    // macOS ignores XDG_CACHE_HOME: start from an empty cache for this test's own
    // app name, or a cache left by an earlier run makes resume() wait instead of fetch.
    QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).removeRecursively();
    using namespace hamclock;
    using namespace std::chrono_literals;

    int calls = 0;
    Transport transport = [&](const QUrl&, FetchDone done) {
        ++calls;
        QTimer::singleShot(0, [done] { done({200, "body", {}}); });
    };
    QDateTime now = QDateTime::currentDateTimeUtc();
    auto& life = AppLifecycle::instance();
    int suspends = 0, resumes = 0;
    QObject::connect(&life, &AppLifecycle::suspended, [&] { ++suspends; });
    QObject::connect(&life, &AppLifecycle::resumed, [&] { ++resumes; });

    PollPolicy policy;
    policy.interval = 600s;
    PolledSource source("lifecycle-test", QUrl("https://x.test/a"), policy, transport);
    source.setClock([&] { return now; });
    source.start();
    spin(app);
    check(calls == 1, "initial fetch");

    // Inactive (system dialog) is not a suspension.
    life.setState(Qt::ApplicationInactive);
    check(!life.isSuspended() && !source.isPaused(), "inactive keeps polling");

    life.setState(Qt::ApplicationSuspended);
    check(life.isSuspended() && source.isPaused() && suspends == 1, "suspend pauses");
    source.refreshNow();
    spin(app);
    check(calls == 1, "no fetch while suspended");
    life.setState(Qt::ApplicationSuspended);
    check(suspends == 1, "repeated suspend is idempotent");

    // Resume soon after: data is fresh, so no immediate fetch.
    now = now.addSecs(60);
    life.setState(Qt::ApplicationActive);
    spin(app);
    check(!source.isPaused() && resumes == 1 && calls == 1, "fresh data not refetched on resume");

    // Resume after the interval elapsed: fetch at once.
    life.setState(Qt::ApplicationSuspended);
    now = now.addSecs(3600);
    life.setState(Qt::ApplicationActive);
    spin(app);
    check(calls == 2, "overdue data refetched on resume");

    // A source created while suspended starts paused and fetches on resume.
    life.setState(Qt::ApplicationSuspended);
    PolledSource late("lifecycle-late", QUrl("https://x.test/b"), policy, transport);
    late.start();
    spin(app);
    check(late.isPaused() && calls == 2, "source created while suspended waits");
    life.setState(Qt::ApplicationActive);
    spin(app);
    check(calls == 3, "late source fetches on resume");

    // Mac sleep/wake: the network is gone while the Mac sleeps, so sleeping counts
    // as suspended and waking resumes (reconnects, refetches when overdue).
    const int suspends_before_sleep = suspends;
    life.setSystemAsleep(true);
    check(life.isSuspended() && source.isPaused() && suspends == suspends_before_sleep + 1, "system sleep suspends");
    now = now.addSecs(3600);
    life.setSystemAsleep(false);
    spin(app);
    check(!life.isSuspended() && !source.isPaused() && calls == 4, "wake resumes and refetches overdue data");

    // Waking while the app itself is still suspended (iPad) keeps it suspended.
    life.setState(Qt::ApplicationSuspended);
    life.setSystemAsleep(true);
    life.setSystemAsleep(false);
    check(life.isSuspended(), "wake does not override an app suspension");
    life.setState(Qt::ApplicationActive);
    check(!life.isSuspended(), "app resume after wake");
    // Sleep reported twice (notification repeats) is one suspension.
    const int suspends_before_repeat = suspends;
    life.setSystemAsleep(true);
    life.setSystemAsleep(true);
    check(suspends == suspends_before_repeat + 1, "repeated sleep is idempotent");
    life.setSystemAsleep(false);

    std::cout << (failures ? "FAILED\n" : "app-lifecycle-regression: ok\n");
    return failures ? 1 : 0;
}
