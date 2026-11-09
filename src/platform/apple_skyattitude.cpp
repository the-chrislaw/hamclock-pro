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

double cameraRollDeg(const RotationMatrix& r) {
    // Forward f = device -Z, up u = device +Y, both in the reference frame (x north, y west, z up).
    const double fx = -r.m31, fy = -r.m32, fz = -r.m33;
    const double ux = r.m21, uy = r.m22, uz = r.m23;
    // Zero-roll up u0 = zenith made orthogonal to f.
    const double ox = -fz * fx, oy = -fz * fy, oz = 1.0 - fz * fz;
    const double on = std::sqrt(ox * ox + oy * oy + oz * oz);
    if (on < 1e-6) return 0;
    // Right-hand side of the view: r0 = f x u0 (u0 normalised below).
    const double rx = fy * oz - fz * oy, ry = fz * ox - fx * oz, rz = fx * oy - fy * ox;
    const double c = (ux * ox + uy * oy + uz * oz) / on;
    const double s = (ux * rx + uy * ry + uz * rz) / on;
    return std::atan2(s, c) * 180.0 / M_PI;
}

} // namespace hamclock
