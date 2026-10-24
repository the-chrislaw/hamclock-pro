#include "apple_skyfinder.hpp"

namespace hamclock {

#ifndef Q_OS_IOS
void nativeHapticTick() {}
#endif

SkyFinder::SkyFinder(SkySensor* sensor, QObject* parent)
    : QObject(parent), sensor_(sensor), haptic_(&nativeHapticTick) {
    connect(sensor_, &SkySensor::attitudeChanged, this, &SkyFinder::onAttitude);
}

void SkyFinder::setTarget(double az, double el) {
    target_az_ = az;
    target_el_ = el;
    have_target_ = true;
}

void SkyFinder::start() {
    if (active_) return;
    sensor_->start();
    if (!sensor_->running()) return; // no motion sensors: stay inactive, not a silent dead 'active'
    active_ = true;
    emit activeChanged();
}

void SkyFinder::stop() {
    if (!active_) return;
    active_ = false;
    sensor_->stop();
    guide_ = PointingGuidance();
    tracker_ = LockTracker();
    emit activeChanged();
    emit guidanceChanged();
}

void SkyFinder::onAttitude(double az, double el) {
    if (!active_ || !have_target_) return;
    guide_ = pointingGuidance(az, el, target_az_, target_el_);
    const bool arrived = tracker_.update(guide_.off_axis_deg);
    emit guidanceChanged();
    if (arrived) {
        if (haptic_) haptic_();
        emit lockedOn();
    }
}

} // namespace hamclock
