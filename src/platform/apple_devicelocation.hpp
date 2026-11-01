#pragma once
// "Use my location" for Settings (macOS/iPad/iPhone): one-shot position fix from
// the OS location service. The acceptance rules are platform-free so they are
// tested on Linux; the native CLLocationManager hook is added separately.
#include <QObject>
#include <QString>

namespace hamclock {

struct LocationFix {
    double lat = 0.0;
    double lon = 0.0;
    double accuracyM = -1.0; // horizontal; negative means the OS says invalid
};

enum class FixVerdict { Accept, BadCoordinate, Invalid, TooCoarse };

// Station grid squares need ~10 km; reject anything coarser (e.g. Wi-Fi/IP fallback).
constexpr double kMaxAccuracyM = 10000.0;

FixVerdict judgeFix(const LocationFix& fix);
QString describeVerdict(FixVerdict v);

class DeviceLocation : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
public:
    explicit DeviceLocation(QObject* parent = nullptr) : QObject(parent) {}

    // True once a native backend is built in; false on Linux/Pi.
    bool available() const { return available_; }
    void setAvailableOverride(bool on) { available_ = on; }

    // Feed a fix from the native layer (or tests). Emits exactly one signal.
    void deliver(const LocationFix& fix);
    void fail(const QString& why);

    // Ask the OS for one fix; the result arrives as located() or failed().
    Q_INVOKABLE void request();

signals:
    void located(double lat, double lon);
    void failed(const QString& reason);

private:
    bool available_ = false;
};

// Native hook (apple_devicelocation_apple.mm on macOS and iOS; a stub elsewhere). Returns false when
// no location service exists; otherwise starts a one-shot request that ends in
// DeviceLocation::deliver() or fail() on the owner's thread.
bool nativeLocationRequest(DeviceLocation* owner);
bool nativeLocationAvailable();

} // namespace hamclock
