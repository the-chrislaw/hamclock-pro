#include "apple_skypointer.hpp"
#include <QCoreApplication>
#include <cmath>

namespace hamclock {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRad = kPi / 180.0;
} // namespace

double wrapSigned(double deg) {
    double d = std::fmod(deg, 360.0);
    if (d > 180.0) d -= 360.0;
    if (d <= -180.0) d += 360.0;
    return d;
}

PointingGuidance pointingGuidance(double phone_az, double phone_el, double target_az, double target_el,
                                  double lock_deg) {
    PointingGuidance g;
    g.turn_deg = wrapSigned(target_az - phone_az);
    g.tilt_deg = target_el - phone_el;
    // Great-circle separation of two directions on the sky.
    const double c = std::sin(phone_el * kRad) * std::sin(target_el * kRad) +
                     std::cos(phone_el * kRad) * std::cos(target_el * kRad) * std::cos(g.turn_deg * kRad);
    g.off_axis_deg = std::acos(std::fmax(-1.0, std::fmin(1.0, c))) / kRad;
    g.locked = g.off_axis_deg <= lock_deg;
    if (g.locked) {
        g.text = QCoreApplication::translate("SkyPointer", "On target");
        return g;
    }
    QStringList parts;
    if (std::fabs(g.turn_deg) >= 1.0)
        parts << (g.turn_deg < 0 ? QCoreApplication::translate("SkyPointer", "Turn left %1°")
                                 : QCoreApplication::translate("SkyPointer", "Turn right %1°"))
                     .arg(qRound(std::fabs(g.turn_deg)));
    if (std::fabs(g.tilt_deg) >= 1.0)
        parts << (g.tilt_deg > 0 ? QCoreApplication::translate("SkyPointer", "tilt up %1°")
                                 : QCoreApplication::translate("SkyPointer", "tilt down %1°"))
                     .arg(qRound(std::fabs(g.tilt_deg)));
    g.text = parts.join(QStringLiteral(", "));
    if (!g.text.isEmpty()) g.text[0] = g.text[0].toUpper();
    return g;
}

void AttitudeSmoother::feed(double az, double el) {
    if (!primed_) {
        az_ = wrapSigned(az) < 0 ? wrapSigned(az) + 360.0 : wrapSigned(az);
        el_ = el;
        primed_ = true;
        return;
    }
    az_ += alpha_ * wrapSigned(az - az_);
    if (az_ < 0) az_ += 360.0;
    if (az_ >= 360.0) az_ -= 360.0;
    el_ += alpha_ * (el - el_);
}

bool LockTracker::update(double off_axis_deg) {
    if (!locked_ && off_axis_deg <= enter_) {
        locked_ = true;
        return true;
    }
    if (locked_ && off_axis_deg > exit_) locked_ = false;
    return false;
}

} // namespace hamclock
