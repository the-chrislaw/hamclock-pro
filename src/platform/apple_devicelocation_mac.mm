#include "apple_devicelocation.hpp"

#import <CoreLocation/CoreLocation.h>

#include <QPointer>

@interface HCLocationDelegate : NSObject <CLLocationManagerDelegate>
@property(nonatomic, assign) QPointer<hamclock::DeviceLocation> owner;
@property(nonatomic, strong) CLLocationManager* manager;
@end

namespace {
HCLocationDelegate* g_delegate = nil;
}

@implementation HCLocationDelegate
- (void)finishWithFix:(const hamclock::LocationFix&)fix error:(NSString*)why {
    hamclock::DeviceLocation* o = self.owner.data();
    if (!o) return;
    if (why) o->fail(QString::fromNSString(why));
    else o->deliver(fix);
}
- (void)locationManager:(CLLocationManager*)m didUpdateLocations:(NSArray<CLLocation*>*)locs {
    CLLocation* l = locs.lastObject;
    if (!l) return;
    hamclock::LocationFix fix;
    fix.lat = l.coordinate.latitude;
    fix.lon = l.coordinate.longitude;
    fix.accuracyM = l.horizontalAccuracy;
    [self finishWithFix:fix error:nil];
}
- (void)locationManager:(CLLocationManager*)m didFailWithError:(NSError*)e {
    [self finishWithFix:hamclock::LocationFix() error:@"The position could not be determined. Check Location Services in System Settings."];
}
- (void)locationManagerDidChangeAuthorization:(CLLocationManager*)m {
    if (m.authorizationStatus == kCLAuthorizationStatusDenied || m.authorizationStatus == kCLAuthorizationStatusRestricted)
        [self finishWithFix:hamclock::LocationFix() error:@"Location access is off for HamClock Pro. Allow it in System Settings > Privacy & Security."];
    else if (m.authorizationStatus == kCLAuthorizationStatusAuthorizedAlways)
        [m requestLocation];
}
@end

namespace hamclock {

bool nativeLocationAvailable() { return [CLLocationManager locationServicesEnabled]; }

bool nativeLocationRequest(DeviceLocation* owner) {
    if (!nativeLocationAvailable()) return false;
    if (!g_delegate) {
        g_delegate = [[HCLocationDelegate alloc] init];
        g_delegate.manager = [[CLLocationManager alloc] init];
        g_delegate.manager.delegate = g_delegate;
        g_delegate.manager.desiredAccuracy = kCLLocationAccuracyKilometer;
    }
    g_delegate.owner = owner;
    CLLocationManager* m = g_delegate.manager;
    if (m.authorizationStatus == kCLAuthorizationStatusNotDetermined) [m requestWhenInUseAuthorization];
    else [m requestLocation];
    return true;
}

} // namespace hamclock
