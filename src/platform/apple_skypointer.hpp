#pragma once
// Pointing maths for "Find it in the sky" (#573). Pure functions with no
// platform code so they are tested on Linux; the iOS sensor feed (CoreMotion
// + CoreLocation heading) only supplies the phone azimuth/elevation.
#include <QString>
#include <QStringList>

namespace hamclock {

// Degrees wrapped into (-180, 180].
double wrapSigned(double deg);

struct PointingGuidance {
    double turn_deg = 0;     // > 0: turn right (clockwise), < 0: turn left
    double tilt_deg = 0;     // > 0: tilt up, < 0: tilt down
    double off_axis_deg = 0; // angular separation between phone and target
    bool locked = false;
    QString text;            // e.g. "Turn left 40°, tilt up 12°" or "On target"
};

// Phone and target az (0 = north, clockwise) and el (horizon = 0, zenith = 90).
PointingGuidance pointingGuidance(double phone_az, double phone_el, double target_az, double target_el,
                                  double lock_deg = 3.0);

// Exponential smoothing of a noisy az/el stream; azimuth is smoothed on the
// shortest arc so it behaves across the 359 -> 0 wrap.
class AttitudeSmoother {
public:
    explicit AttitudeSmoother(double alpha = 0.25) : alpha_(alpha) {}
    void feed(double az, double el);
    bool primed() const { return primed_; }
    double az() const { return az_; }
    double el() const { return el_; }
    void reset() { primed_ = false; }
private:
    double alpha_;
    bool primed_ = false;
    double az_ = 0, el_ = 0;
};

// Lock indicator with hysteresis so the haptic tick fires once on arrival and
// does not chatter at the edge.
class LockTracker {
public:
    LockTracker(double enter_deg = 3.0, double exit_deg = 5.0) : enter_(enter_deg), exit_(exit_deg) {}
    // Returns true only on the update that enters the locked state.
    bool update(double off_axis_deg);
    bool locked() const { return locked_; }
private:
    double enter_, exit_;
    bool locked_ = false;
};

} // namespace hamclock
