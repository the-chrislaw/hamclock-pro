#pragma once

#include "../domain/geometry.hpp"
#include <QString>
#include <cmath>

namespace hamclock {

// DE == DX gives "0 KM" and "BEARING UNDEFINED", which reads as broken.
// Show a prompt instead; any other distance/bearing pair is shown as is.
inline QString path_summary(const QString& distance, const QString& bearing) {
    if (bearing.contains("UNDEFINED", Qt::CaseInsensitive)) return "Set a DX target in Settings";
    return distance + "  ·  " + bearing;
}

// Short or long great-circle path, "Short path\n8123 KM\n45° TRUE".
// Antipodal/coincident pairs have no bearing and fall back to the prompt.
inline QString path_detail(const GeometryResult& g, bool long_path, bool imperial = false) {
    if (g.status != BearingStatus::Defined || std::isnan(g.short_bearing_deg))
        return path_summary("0 KM", "BEARING UNDEFINED");
    const double km = long_path ? g.long_distance_km : g.distance_km;
    const double deg = long_path ? g.long_bearing_deg : g.short_bearing_deg;
    // Three deliberate lines (path, distance, bearing) so nothing wraps mid-value or leaves a separator; no-break spaces keep value and unit together.
    const QChar nb(0x00A0);
    return QString("%1\n%2%3%4\n%5°%3TRUE")
        .arg(long_path ? "Long path" : "Short path")
        .arg(qRound(imperial ? km / 1.609344 : km)).arg(nb).arg(imperial ? "MI" : "KM").arg(qRound(deg) % 360);
}

} // namespace hamclock
