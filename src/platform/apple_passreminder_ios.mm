#include "apple_passreminder.hpp"

#import <UserNotifications/UserNotifications.h>

namespace hamclock {

bool nativeSchedulePassReminder(const ReminderPlan& plan) {
    if (!plan.valid) return false;
    UNUserNotificationCenter* center = UNUserNotificationCenter.currentNotificationCenter;
    // Asked only when a reminder is first scheduled, never at launch.
    [center requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound)
                          completionHandler:^(BOOL, NSError*) {}];
    UNMutableNotificationContent* c = [UNMutableNotificationContent new];
    c.title = plan.title.toNSString();
    c.body = plan.body.toNSString();
    c.sound = UNNotificationSound.defaultSound;
    const NSTimeInterval secs = plan.fire_utc.toSecsSinceEpoch() - NSDate.date.timeIntervalSince1970;
    if (secs < 1) return false;
    UNTimeIntervalNotificationTrigger* t =
        [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:secs repeats:NO];
    UNNotificationRequest* r = [UNNotificationRequest requestWithIdentifier:plan.id.toNSString() content:c trigger:t];
    [center addNotificationRequest:r withCompletionHandler:nil];
    return true;
}

void nativeCancelPassReminder(const QString& id) {
    [UNUserNotificationCenter.currentNotificationCenter removePendingNotificationRequestsWithIdentifiers:@[ id.toNSString() ]];
}

} // namespace hamclock
