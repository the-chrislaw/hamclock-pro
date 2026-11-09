#include "apple_camerafov.hpp"

#import <AVFoundation/AVFoundation.h>
#include <QMetaObject>
#include <QCoreApplication>

namespace hamclock {

// Long-side field of view (degrees) of the active format of the default rear
// wide-angle camera, or 0 when there is no such camera (Simulator, Mac).
double nativeCameraLongFovDeg(QSizeF *sensor_px) {
    AVCaptureDevice *dev = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInWideAngleCamera
                                                              mediaType:AVMediaTypeVideo
                                                               position:AVCaptureDevicePositionBack];
    if (!dev)
        return 0;
    AVCaptureDeviceFormat *fmt = dev.activeFormat;
    if (sensor_px) {
        CMVideoDimensions d = CMVideoFormatDescriptionGetDimensions(fmt.formatDescription);
        *sensor_px = QSizeF(d.width, d.height);
    }
    return fmt.videoFieldOfView;
}

void nativeCameraRequestAccess(std::function<void(bool)> done) {
    auto finish = [done](bool ok) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [done, ok] { done(ok); }, Qt::QueuedConnection);
    };
    switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo]) {
    case AVAuthorizationStatusAuthorized: finish(true); break;
    case AVAuthorizationStatusNotDetermined:
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) { finish(granted); }];
        break;
    default: finish(false); break;
    }
}

} // namespace hamclock
