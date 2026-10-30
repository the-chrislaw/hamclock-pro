#include "apple_widgetshare.hpp"
#include <cmath>

namespace hamclock {

namespace {
bool valid_position(double lat, double lon) {
    return std::isfinite(lat) && std::isfinite(lon) && lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180;
}
} // namespace

WidgetShare makeWidgetShare(const StationSettings& station, bool configured, const QStringList& tracked) {
    WidgetShare out;
    for (const QString& raw : tracked) {
        const QString name = raw.trimmed();
        if (name.isEmpty() || name.size() > kWidgetMaxNameLength) continue;
        if (out.tracked.contains(name, Qt::CaseInsensitive)) continue;
        out.tracked << name;
        if (out.tracked.size() >= kWidgetMaxTracked) break;
    }
    if (!configured || !valid_position(station.de.latitude, station.de.longitude)) return out;
    out.configured = true;
    out.callsign = QString::fromStdString(station.callsign).trimmed().toUpper().left(kWidgetMaxNameLength);
    out.latitude = station.de.latitude;
    out.longitude = station.de.longitude;
    return out;
}

QVariantMap widgetShareFields(const WidgetShare& share, qint64 now_s) {
    QVariantMap m;
    m["share.version"] = kWidgetShareVersion;
    m["share.updated"] = now_s;
    m["station.configured"] = share.configured;
    m["station.callsign"] = share.configured ? share.callsign : QString();
    m["station.lat"] = share.configured ? share.latitude : 0.0;
    m["station.lon"] = share.configured ? share.longitude : 0.0;
    m["satellites.tracked"] = share.tracked;
    return m;
}

#ifndef Q_OS_IOS
bool nativeShareForWidgets(const WidgetShare&) { return false; }
#endif

} // namespace hamclock
