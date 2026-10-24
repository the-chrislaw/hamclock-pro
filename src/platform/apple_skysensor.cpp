#include "apple_skysensor.hpp"

namespace hamclock {

#ifndef Q_OS_IOS
bool nativeSkySensorAvailable() { return false; }
bool nativeSkySensorStart(SkySensor*, void (*)(SkySensor*, const RotationMatrix&)) { return true; }
void nativeSkySensorStop(SkySensor*) {}
#endif

SkySensor::SkySensor(QObject* parent) : QObject(parent) {}

SkySensor::~SkySensor() { stop(); }

bool SkySensor::available() const { return available_override_ || nativeSkySensorAvailable(); }

void SkySensor::start() {
    if (running_ || !available()) return;
    smoother_.reset();
    running_ = true;
    true_north_ = nativeSkySensorStart(this, [](SkySensor* self, const RotationMatrix& r) { self->feedAttitude(r); });
    emit runningChanged();
}

void SkySensor::stop() {
    if (!running_) return;
    running_ = false;
    nativeSkySensorStop(this);
    emit runningChanged();
}

void SkySensor::feedAttitude(const RotationMatrix& r) {
    const AzEl a = attitudeToAzEl(r, axis_);
    smoother_.feed(a.az, a.el);
    emit attitudeChanged(smoother_.az(), smoother_.el());
}

} // namespace hamclock
