#include "apple_skysensor.hpp"

#import <CoreLocation/CoreLocation.h>
#import <CoreMotion/CoreMotion.h>

namespace hamclock {

namespace {
CMMotionManager* g_motion = nil;
CLLocationManager* g_location = nil;
SkySensor* g_owner = nullptr; // cleared on stop so queued samples are dropped
CMMotionManager* motionManager() {
    if (!g_motion) g_motion = [[CMMotionManager alloc] init];
    return g_motion;
}
} // namespace

bool nativeSkySensorAvailable() {
    return motionManager().deviceMotionAvailable;
}

bool nativeSkySensorStart(SkySensor* owner, void (*deliver)(SkySensor*, const RotationMatrix&),
                           void (*deliverAccuracy)(SkySensor*, double)) {
    // True-north reference needs location services; ask only when the finder starts.
    if (!g_location) g_location = [[CLLocationManager alloc] init];
    if (g_location.authorizationStatus == kCLAuthorizationStatusNotDetermined)
        [g_location requestWhenInUseAuthorization];

    if (!motionManager().deviceMotionAvailable) return true;
    g_owner = owner;
    g_motion.deviceMotionUpdateInterval = 1.0 / 30.0;
    CMAttitudeReferenceFrame frame = CMAttitudeReferenceFrameXTrueNorthZVertical;
    const bool trueNorth = ([CMMotionManager availableAttitudeReferenceFrames] & frame) != 0;
    if (!trueNorth) frame = CMAttitudeReferenceFrameXMagneticNorthZVertical;
    [g_motion startDeviceMotionUpdatesUsingReferenceFrame:frame
                                                  toQueue:NSOperationQueue.mainQueue
                                              withHandler:^(CMDeviceMotion* motion, NSError*) {
        if (!motion || g_owner != owner) return;
        const CMRotationMatrix m = motion.attitude.rotationMatrix;
        RotationMatrix r;
        r.m11 = m.m11; r.m12 = m.m12; r.m13 = m.m13;
        r.m21 = m.m21; r.m22 = m.m22; r.m23 = m.m23;
        r.m31 = m.m31; r.m32 = m.m32; r.m33 = m.m33;
        deliver(owner, r);
        // Magnetometer calibration level -> approximate heading error in degrees.
        double acc = -1;
        switch (motion.magneticField.accuracy) {
        case CMMagneticFieldCalibrationAccuracyHigh: acc = 10; break;
        case CMMagneticFieldCalibrationAccuracyMedium: acc = 25; break;
        case CMMagneticFieldCalibrationAccuracyLow: acc = 45; break;
        default: break; // uncalibrated: unknown
        }
        deliverAccuracy(owner, acc);
    }];
    return trueNorth;
}

void nativeSkySensorStop(SkySensor* owner) {
    if (g_owner == owner) g_owner = nullptr;
    [g_motion stopDeviceMotionUpdates];
}

} // namespace hamclock
