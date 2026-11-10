#pragma once
// Effective field of view of an aspect-fill camera preview (#573 item 3). The
// iOS camera layer reports the sensor format's long-side field of view
// (AVCaptureDeviceFormat.videoFieldOfView); the preview crops it to fill the
// view, so the projection needs the cropped values. Pure maths, tested on Linux.
#include <QSizeF>
#include <functional>

namespace hamclock {

struct PreviewFov {
    double hfov_deg = 60; // across the preview width
    double vfov_deg = 45; // across the preview height
};

// sensor_long_fov_deg: field of view along the sensor's long side.
// sensor_px: sensor/format size (e.g. 4032x3024), orientation ignored.
// preview: the preview rectangle; portrait previews rotate the sensor.
// Invalid input (fov outside (1, 170) or empty sizes) returns the defaults.
PreviewFov previewFov(double sensor_long_fov_deg, const QSizeF &sensor_px, const QSizeF &preview);

#ifdef HAMCLOCK_SKY_FINDER
// iOS only (apple_camera_ios.mm). Returns 0 when no rear camera is available.
double nativeCameraLongFovDeg(QSizeF *sensor_px);

// Ask for camera permission (shows the system prompt the first time). `done(granted)` is
// called on the main thread. Only call when the user turns the camera view on.
void nativeCameraRequestAccess(std::function<void(bool)> done);
#endif

} // namespace hamclock
