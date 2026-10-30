#include "apple_devicelocation.hpp"

#include <QtGlobal>
#include <cmath>

namespace hamclock {

FixVerdict judgeFix(const LocationFix& fix) {
    if (!std::isfinite(fix.lat) || !std::isfinite(fix.lon) || fix.lat < -90.0 || fix.lat > 90.0 ||
        fix.lon < -180.0 || fix.lon > 180.0)
        return FixVerdict::BadCoordinate;
    if (!std::isfinite(fix.accuracyM) || fix.accuracyM < 0.0) return FixVerdict::Invalid;
    if (fix.accuracyM > kMaxAccuracyM) return FixVerdict::TooCoarse;
    return FixVerdict::Accept;
}

QString describeVerdict(FixVerdict v) {
    switch (v) {
    case FixVerdict::Accept: return QString();
    case FixVerdict::BadCoordinate: return QStringLiteral("The device reported an impossible position.");
    case FixVerdict::Invalid: return QStringLiteral("No position is available yet. Try again outdoors or near a window.");
    case FixVerdict::TooCoarse: return QStringLiteral("The position is too rough for a grid square. Try again with Location Services precise.");
    }
    return QString();
}

void DeviceLocation::deliver(const LocationFix& fix) {
    const FixVerdict v = judgeFix(fix);
    if (v == FixVerdict::Accept) emit located(fix.lat, fix.lon);
    else emit failed(describeVerdict(v));
}

void DeviceLocation::fail(const QString& why) { emit failed(why); }

void DeviceLocation::request() {
    if (!nativeLocationRequest(this)) fail(QStringLiteral("Location Services are not available on this device."));
}

#ifndef Q_OS_MACOS
bool nativeLocationRequest(DeviceLocation*) { return false; }
bool nativeLocationAvailable() { return false; }
#endif

} // namespace hamclock
