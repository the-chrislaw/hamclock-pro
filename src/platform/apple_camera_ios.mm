#include "apple_camerafov.hpp"

#import <AVFoundation/AVFoundation.h>

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

} // namespace hamclock
