#pragma once
// Effective field of view of an aspect-fill camera preview (#573 item 3). The
// iOS camera layer reports the sensor format's long-side field of view
// (AVCaptureDeviceFormat.videoFieldOfView); the preview crops it to fill the
// view, so the projection needs the cropped values. Pure maths, tested on Linux.
#include <QSizeF>

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

} // namespace hamclock
