#include "apple_skyattitude.hpp"

#include <algorithm>
#include <cmath>

namespace hamclock {

AzEl attitudeToAzEl(const RotationMatrix& r, PointingAxis axis) {
    // v_ref = R^T * v_device
    double x, y, z;
    if (axis == PointingAxis::BackCamera) {
        x = -r.m31; y = -r.m32; z = -r.m33;
    } else {
        x = r.m21; y = r.m22; z = r.m23;
    }
    const double rad = 180.0 / M_PI;
    AzEl out;
    out.el = std::asin(std::clamp(z, -1.0, 1.0)) * rad;
    if (std::hypot(x, y) < 1e-9) {
        out.az = 0; // straight up or down: azimuth is undefined
        return out;
    }
    double az = std::atan2(-y, x) * rad; // y points west, so east = -y
    if (az < 0) az += 360.0;
    if (az >= 360.0) az -= 360.0;
    out.az = az;
    return out;
}

} // namespace hamclock
