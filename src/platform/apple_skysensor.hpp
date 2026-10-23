#pragma once
// Phone pointing direction for "Find it in the sky" (#573): CoreMotion device
// attitude relative to true north on iOS, unavailable elsewhere. Emits
// smoothed azimuth/elevation of the chosen device axis.
#include "apple_skyattitude.hpp"
#include "apple_skypointer.hpp"

#include <QObject>

namespace hamclock {

class SkySensor : public QObject {
    Q_OBJECT
public:
    explicit SkySensor(QObject* parent = nullptr);
    ~SkySensor() override;

    // True on iOS devices with motion sensors; false on the simulator, macOS, Pi.
    bool available() const;
    bool running() const { return running_; }
    void setAxis(PointingAxis axis) { axis_ = axis; smoother_.reset(); }

    void start();
    void stop();

    // Feed one raw attitude sample (called by the native layer, or by tests).
    void feedAttitude(const RotationMatrix& r);

signals:
    void attitudeChanged(double az, double el);
    void runningChanged();

private:
    bool running_ = false;
    PointingAxis axis_ = PointingAxis::BackCamera;
    AttitudeSmoother smoother_;
};

// Native hooks, implemented in apple_skysensor_ios.mm on iOS only.
bool nativeSkySensorAvailable();
void nativeSkySensorStart(SkySensor* owner, void (*deliver)(SkySensor*, const RotationMatrix&));
void nativeSkySensorStop(SkySensor* owner);

} // namespace hamclock
