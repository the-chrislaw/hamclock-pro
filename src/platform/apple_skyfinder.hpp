#pragma once
// "Find it in the sky" controller (#573): joins the phone attitude feed with a
// target az/el (from the pointing model, #572) and exposes guidance text plus a
// one-shot haptic tick on lock. No platform code here, so it is tested on Linux.
#include "apple_skysensor.hpp"

#include <QObject>
#include <QString>
#include <functional>

namespace hamclock {

class SkyFinder : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString guidance READ guidance NOTIFY guidanceChanged)
    Q_PROPERTY(bool locked READ locked NOTIFY guidanceChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(bool magneticNorth READ magneticNorth NOTIFY activeChanged)
public:
    explicit SkyFinder(SkySensor* sensor, QObject* parent = nullptr);

    QString guidance() const { return guide_.text; }
    bool locked() const { return tracker_.locked(); }
    bool active() const { return active_; }
    bool magneticNorth() const { return active_ && !sensor_->trueNorth(); }
    PointingGuidance current() const { return guide_; }

    // Replaces the haptic (default: nativeHapticTick, a no-op off iOS).
    void setHaptic(std::function<void()> fn) { haptic_ = std::move(fn); }

    Q_INVOKABLE void setTarget(double az, double el);
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

signals:
    void guidanceChanged();
    void activeChanged();
    void lockedOn(); // emitted once per arrival, together with the haptic

private slots:
    void onAttitude(double az, double el);

private:
    SkySensor* sensor_;
    double target_az_ = 0, target_el_ = 0;
    bool have_target_ = false;
    bool active_ = false;
    PointingGuidance guide_;
    LockTracker tracker_;
    std::function<void()> haptic_;
};

// Light impact tick; implemented with UIImpactFeedbackGenerator on iOS only.
void nativeHapticTick();

} // namespace hamclock
