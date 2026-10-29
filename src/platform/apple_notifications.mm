// UNUserNotificationCenter bridge for iPhone, iPad and the Mac app.
#include "apple_notifications.hpp"

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>

#include <QSet>

// Shows our notifications as banners (with sound) while the app is open; iOS
// otherwise drops them silently when the app is in front.
@interface HCNotificationPresenter : NSObject <UNUserNotificationCenterDelegate>
@end

@implementation HCNotificationPresenter
- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler {
    (void)center;
    (void)notification;
    completionHandler(UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList |
                      UNNotificationPresentationOptionSound);
}
- (void)userNotificationCenter:(UNUserNotificationCenter*)center
    didReceiveNotificationResponse:(UNNotificationResponse*)response
             withCompletionHandler:(void (^)(void))completionHandler {
    (void)center;
    (void)response;
    completionHandler(); // a tap just opens the app
}
@end

namespace hamclock::notify {
namespace {

// UNUserNotificationCenter raises an exception outside an app bundle (unit
// tests, a bare build-tree binary), so those report "unsupported".
bool bundled() {
    NSBundle* main = NSBundle.mainBundle;
    return main.bundleIdentifier.length > 0 && [main.bundlePath hasSuffix:@".app"];
}

Permission from_status(UNAuthorizationStatus status) {
    switch (status) {
    case UNAuthorizationStatusAuthorized:
    case UNAuthorizationStatusProvisional:
        return Permission::Granted;
    case UNAuthorizationStatusDenied: return Permission::Denied;
    case UNAuthorizationStatusNotDetermined: return Permission::NotDetermined;
    default: return Permission::Granted; // ephemeral (App Clips): can post
    }
}

template <typename F> void on_main(F f) { dispatch_async(dispatch_get_main_queue(), ^{ f(); }); }

UNNotificationRequest* make_request(const LocalAlert& alert) {
    const NSTimeInterval secs = alert.fire_utc.toSecsSinceEpoch() - NSDate.date.timeIntervalSince1970;
    if (secs < 1 || alert.id.isEmpty()) return nil;
    UNMutableNotificationContent* content = [UNMutableNotificationContent new];
    content.title = alert.title.toNSString();
    content.body = alert.body.toNSString();
    if (alert.sound) content.sound = UNNotificationSound.defaultSound;
    content.threadIdentifier = @"satellite-passes";
    UNTimeIntervalNotificationTrigger* trigger = [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:secs
                                                                                                    repeats:NO];
    return [UNNotificationRequest requestWithIdentifier:alert.id.toNSString() content:content trigger:trigger];
}

NSArray<NSString*>* ids_with_prefix(NSArray<UNNotificationRequest*>* requests, NSString* prefix) {
    NSMutableArray<NSString*>* out = [NSMutableArray array];
    for (UNNotificationRequest* r in requests)
        if ([r.identifier hasPrefix:prefix]) [out addObject:r.identifier];
    return out;
}

HCNotificationPresenter* presenter = nil; // strong: the centre keeps only a weak delegate

} // namespace

bool supported() { return bundled(); }

void install_foreground_presenter() {
    if (!bundled() || presenter) return;
    presenter = [HCNotificationPresenter new];
    UNUserNotificationCenter.currentNotificationCenter.delegate = presenter;
}

void query_permission(std::function<void(Permission)> done) {
    if (!bundled()) { if (done) done(Permission::Unsupported); return; }
    [UNUserNotificationCenter.currentNotificationCenter
        getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings) {
            const Permission p = from_status(settings.authorizationStatus);
            on_main([done, p] { if (done) done(p); });
        }];
}

void request_permission(std::function<void(Permission)> done) {
    if (!bundled()) { if (done) done(Permission::Unsupported); return; }
    [UNUserNotificationCenter.currentNotificationCenter
        requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound)
                      completionHandler:^(BOOL, NSError*) { query_permission(done); }];
}

void pending_ids(const QString& prefix, std::function<void(QStringList)> done) {
    if (!bundled()) { if (done) done({}); return; }
    NSString* p = prefix.toNSString(); // blocks below capture copies, never the reference
    [UNUserNotificationCenter.currentNotificationCenter
        getPendingNotificationRequestsWithCompletionHandler:^(NSArray<UNNotificationRequest*>* requests) {
            QStringList ids;
            for (NSString* identifier in ids_with_prefix(requests, p)) ids.append(QString::fromNSString(identifier));
            ids.sort();
            on_main([done, ids] { if (done) done(ids); });
        }];
}

void replace_alerts(const QString& prefix, const std::vector<LocalAlert>& alerts, std::function<void(int)> done) {
    if (!bundled()) { if (done) done(0); return; }
    UNUserNotificationCenter* center = UNUserNotificationCenter.currentNotificationCenter;
    NSMutableArray<UNNotificationRequest*>* requests = [NSMutableArray array];
    NSMutableSet<NSString*>* keep = [NSMutableSet set];
    for (const LocalAlert& alert : alerts) {
        if (UNNotificationRequest* r = make_request(alert)) {
            [requests addObject:r];
            [keep addObject:r.identifier];
        }
    }
    NSString* p = prefix.toNSString();
    const QString prefix_copy = prefix; // blocks capture values; never capture the reference parameter
    [center getPendingNotificationRequestsWithCompletionHandler:^(NSArray<UNNotificationRequest*>* pending) {
        NSMutableArray<NSString*>* stale = [NSMutableArray array];
        for (NSString* identifier in ids_with_prefix(pending, p))
            if (![keep containsObject:identifier]) [stale addObject:identifier];
        if (stale.count) [center removePendingNotificationRequestsWithIdentifiers:stale];
        dispatch_group_t group = dispatch_group_create();
        for (UNNotificationRequest* r in requests) {
            dispatch_group_enter(group);
            [center addNotificationRequest:r withCompletionHandler:^(NSError*) { dispatch_group_leave(group); }];
        }
        dispatch_group_notify(group, dispatch_get_main_queue(), ^{ pending_ids(prefix_copy, [done](QStringList ids) {
            if (done) done(static_cast<int>(ids.size()));
        }); });
    }];
}

void cancel_alerts(const QString& prefix, std::function<void(int)> done) {
    if (!bundled()) { if (done) done(0); return; }
    UNUserNotificationCenter* center = UNUserNotificationCenter.currentNotificationCenter;
    NSString* p = prefix.toNSString();
    const QString prefix_copy = prefix;
    [center getPendingNotificationRequestsWithCompletionHandler:^(NSArray<UNNotificationRequest*>* pending) {
        NSArray<NSString*>* ours = ids_with_prefix(pending, p);
        if (ours.count) [center removePendingNotificationRequestsWithIdentifiers:ours];
        on_main([prefix_copy, done] { pending_ids(prefix_copy, [done](QStringList ids) { if (done) done(static_cast<int>(ids.size())); }); });
    }];
}

} // namespace hamclock::notify
