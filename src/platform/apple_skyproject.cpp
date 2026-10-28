#include "apple_skyproject.hpp"

#include <cmath>

namespace hamclock {

namespace {
constexpr double kDeg = M_PI / 180.0;
}

ScreenPoint projectToScreen(const CameraView &view, double target_az, double target_el, const QSizeF &screen)
{
    ScreenPoint out;
    if (screen.width() <= 0 || screen.height() <= 0 || view.hfov_deg <= 0 || view.hfov_deg >= 180 ||
        view.vfov_deg <= 0 || view.vfov_deg >= 180)
        return out;

    // East-north-up unit vectors.
    const double ta = target_az * kDeg, te = target_el * kDeg;
    const double va = view.az_deg * kDeg, ve = view.el_deg * kDeg;
    const double tx = std::sin(ta) * std::cos(te), ty = std::cos(ta) * std::cos(te), tz = std::sin(te);
    const double fx = std::sin(va) * std::cos(ve), fy = std::cos(va) * std::cos(ve), fz = std::sin(ve);
    const double rx = std::cos(va), ry = -std::sin(va); // right = horizontal, 90 deg clockwise of forward
    const double ux = ry * fz - 0 * fy, uy = 0 * fx - rx * fz, uz = rx * fy - ry * fx; // right x forward

    double x = tx * rx + ty * ry;
    double y = tx * ux + ty * uy + tz * uz;
    const double z = tx * fx + ty * fy + tz * fz;

    const double roll = view.roll_deg * kDeg; // image turns clockwise, so scene turns counter-clockwise
    const double xr = x * std::cos(roll) - y * std::sin(roll);
    const double yr = x * std::sin(roll) + y * std::cos(roll);
    x = xr;
    y = yr;

    out.edge_angle_deg = std::fmod(std::atan2(x, y) / kDeg + 360.0, 360.0);
    if (z <= 1e-6)
        return out;
    out.in_front = true;
    const double nx = x / z / std::tan(view.hfov_deg * kDeg / 2);
    const double ny = y / z / std::tan(view.vfov_deg * kDeg / 2);
    out.pos = QPointF((nx + 1) / 2 * screen.width(), (1 - ny) / 2 * screen.height());
    out.on_screen = std::fabs(nx) <= 1 && std::fabs(ny) <= 1;
    return out;
}

QVector<ScreenPoint> projectTrack(const CameraView &view, const QVector<QPointF> &az_el, const QSizeF &screen)
{
    QVector<ScreenPoint> out;
    out.reserve(az_el.size());
    for (const QPointF &p : az_el) {
        const ScreenPoint s = projectToScreen(view, p.x(), p.y(), screen);
        if (s.in_front)
            out.push_back(s);
    }
    return out;
}

} // namespace hamclock
