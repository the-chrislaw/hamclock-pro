#include "apple_skysensor.hpp"

#import <CoreLocation/CoreLocation.h>
#import <CoreMotion/CoreMotion.h>

namespace hamclock {

namespace {
CMMotionManager* g_motion = nil;
CLLocationManager* g_location = nil;
} // namespace

bool nativeSkySensorAvailable() {
    CMMotionManager* m = [[CMMotionManager alloc] init];
    return m.deviceMotionAvailable;
}

void nativeSkySensorStart(SkySensor* owner, void (*deliver)(SkySensor*, const RotationMatrix&)) {
    // True-north reference needs location services; ask only when the finder starts.
    if (!g_location) g_location = [[CLLocationManager alloc] init];
    if (g_location.authorizationStatus == kCLAuthorizationStatusNotDetermined)
        [g_location requestWhenInUseAuthorization];

    if (!g_motion) g_motion = [[CMMotionManager alloc] init];
    if (!g_motion.deviceMotionAvailable) return;
    g_motion.deviceMotionUpdateInterval = 1.0 / 30.0;
    CMAttitudeReferenceFrame frame = CMAttitudeReferenceFrameXTrueNorthZVertical;
    if (([CMMotionManager availableAttitudeReferenceFrames] & frame) == 0)
        frame = CMAttitudeReferenceFrameXMagneticNorthZVertical;
    [g_motion startDeviceMotionUpdatesUsingReferenceFrame:frame
                                                  toQueue:NSOperationQueue.mainQueue
                                              withHandler:^(CMDeviceMotion* motion, NSError*) {
        if (!motion) return;
        const CMRotationMatrix m = motion.attitude.rotationMatrix;
        RotationMatrix r;
        r.m11 = m.m11; r.m12 = m.m12; r.m13 = m.m13;
        r.m21 = m.m21; r.m22 = m.m22; r.m23 = m.m23;
        r.m31 = m.m31; r.m32 = m.m32; r.m33 = m.m33;
        deliver(owner, r);
    }];
}

void nativeSkySensorStop(SkySensor*) {
    [g_motion stopDeviceMotionUpdates];
}

} // namespace hamclock
