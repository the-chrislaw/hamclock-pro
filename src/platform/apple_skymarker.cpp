#include "apple_skymarker.hpp"

namespace hamclock {

SkyMarker::SkyMarker(SkySensor* sensor, SkyFinder* finder, QObject* parent)
    : QObject(parent), finder_(finder) {
    connect(sensor, &SkySensor::attitudeChanged, this, &SkyMarker::onAttitude);
    connect(finder, &SkyFinder::targetChanged, this, &SkyMarker::recompute);
}

void SkyMarker::setPreview(double width, double height, double hfov_deg, double vfov_deg) {
    size_ = QSizeF(width, height);
    view_.hfov_deg = hfov_deg;
    view_.vfov_deg = vfov_deg;
    recompute();
}

void SkyMarker::onAttitude(double az, double el) {
    view_.az_deg = az;
    view_.el_deg = el;
    have_attitude_ = true;
    recompute();
}

void SkyMarker::recompute() {
    ScreenPoint next;
    if (have_attitude_ && finder_->haveTarget() && size_.width() > 0 && size_.height() > 0)
        next = projectToScreen(view_, finder_->targetAz(), finder_->targetEl(), size_);
    const bool same = next.in_front == pt_.in_front && next.on_screen == pt_.on_screen &&
                      next.pos == pt_.pos && next.edge_angle_deg == pt_.edge_angle_deg;
    pt_ = next;
    if (!same) emit changed();
}

} // namespace hamclock
