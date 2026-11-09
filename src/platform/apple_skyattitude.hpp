#pragma once
// Device attitude -> pointing direction for "Find it in the sky" (#573).
// Pure maths so it is tested on Linux; the iOS feed (apple_skysensor_ios.mm)
// passes in CoreMotion's rotation matrix.
#include <functional>

namespace hamclock {

// Row-major 3x3 rotation matrix as CMRotationMatrix (m11..m33): maps vectors
// from the reference frame to the device frame. The reference frame is
// CMAttitudeReferenceFrameXTrueNorthZVertical: x = true north, y = west, z = up.
struct RotationMatrix {
    double m11 = 1, m12 = 0, m13 = 0;
    double m21 = 0, m22 = 1, m23 = 0;
    double m31 = 0, m32 = 0, m33 = 1;
};

enum class PointingAxis {
    BackCamera, // device -Z: the direction the rear camera looks
    TopEdge     // device +Y: where the top edge of a hand-held phone points
};

struct AzEl {
    double az = 0; // degrees clockwise from true north, [0, 360)
    double el = 0; // degrees above the horizon, [-90, 90]
};

AzEl attitudeToAzEl(const RotationMatrix& r, PointingAxis axis);

// Roll of the phone about the rear camera's viewing axis, degrees in (-180, 180]:
// 0 when the top edge points at the sky (level horizon), > 0 when the phone is
// turned clockwise as seen by the user. 0 when looking straight up or down,
// where roll is undefined.
double cameraRollDeg(const RotationMatrix& r);

} // namespace hamclock
