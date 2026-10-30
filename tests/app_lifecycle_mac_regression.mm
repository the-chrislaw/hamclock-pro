// macOS: system sleep/wake notifications reach AppLifecycle once the app attaches,
// so polling and connections stop before the Mac sleeps and restart after wake.
#include "../src/platform/apple_lifecycle.hpp"

#import <AppKit/AppKit.h>
#include <QCoreApplication>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
void post(NSNotificationName name) {
    [NSWorkspace.sharedWorkspace.notificationCenter postNotificationName:name object:NSWorkspace.sharedWorkspace];
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    auto& life = hamclock::AppLifecycle::instance();
    int suspends = 0, resumes = 0;
    QObject::connect(&life, &hamclock::AppLifecycle::suspended, [&] { ++suspends; });
    QObject::connect(&life, &hamclock::AppLifecycle::resumed, [&] { ++resumes; });

    post(NSWorkspaceWillSleepNotification);
    check(!life.isSuspended(), "not attached: sleep ignored");

    life.attachToApplication();
    life.attachToApplication(); // attaching twice must not double the observers
    check(!life.isSuspended(), "awake after attach");
    post(NSWorkspaceWillSleepNotification);
    check(life.isSuspended() && suspends == 1, "will-sleep suspends");
    post(NSWorkspaceDidWakeNotification);
    check(!life.isSuspended() && resumes == 1, "did-wake resumes");

    std::cout << (failures ? "FAILED\n" : "app-lifecycle-mac-regression: ok\n");
    return failures ? 1 : 0;
}
