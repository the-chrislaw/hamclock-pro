// macOS: treat system sleep as a suspension. While the Mac sleeps its network goes
// away; TCP connections (DX cluster, APRS-IS, rigctld) look open after wake but are
// dead, so without this the spot lists stop updating until the OS times them out.
#include "apple_lifecycle.hpp"

#import <AppKit/AppKit.h>

namespace hamclock {

void AppLifecycle::attachToSystemSleep() {
    static bool attached = false;
    if (attached) return;
    attached = true;
    NSNotificationCenter* center = NSWorkspace.sharedWorkspace.notificationCenter;
    // queue nil: run on the posting thread. NSWorkspace posts on the main (GUI) thread.
    [center addObserverForName:NSWorkspaceWillSleepNotification object:nil queue:nil
                    usingBlock:^(NSNotification*) { AppLifecycle::instance().setSystemAsleep(true); }];
    [center addObserverForName:NSWorkspaceDidWakeNotification object:nil queue:nil
                    usingBlock:^(NSNotification*) { AppLifecycle::instance().setSystemAsleep(false); }];
}

} // namespace hamclock
