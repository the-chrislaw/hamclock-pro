#pragma once
// Camera AR projection for "Find it in the sky" (#573 item 3). Pure maths, no
// platform code, so it is tested on Linux; the iOS camera layer only supplies
// the phone attitude and the camera's field of view.
#include <QPointF>
#include <QSizeF>
#include <QVector>

namespace hamclock {

struct CameraView {
    double az_deg = 0;     // direction the camera looks at (0 = north, clockwise)
    double el_deg = 0;     // horizon = 0, zenith = 90
    double roll_deg = 0;   // rotation about the view axis, > 0 turns the image clockwise
    double hfov_deg = 60;  // horizontal field of view of the preview
    double vfov_deg = 45;  // vertical field of view of the preview
};

struct ScreenPoint {
    bool in_front = false;   // target is ahead of the camera (z > 0)
    bool on_screen = false;  // in front and inside the preview rectangle
    QPointF pos;             // pixels, origin top-left; valid when in_front
    double edge_angle_deg = 0; // direction to the target on screen, 0 = up, clockwise; for an off-screen arrow
};

ScreenPoint projectToScreen(const CameraView &view, double target_az, double target_el, const QSizeF &screen);

// Projects a pass track (az, el pairs); points behind the camera are skipped.
QVector<ScreenPoint> projectTrack(const CameraView &view, const QVector<QPointF> &az_el, const QSizeF &screen);

} // namespace hamclock
