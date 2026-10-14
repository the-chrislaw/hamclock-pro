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

// Short or long great-circle path, "Short path · 8123 KM  ·  45 DEG / TRUE".
// Antipodal/coincident pairs have no bearing and fall back to the prompt.
inline QString path_detail(const GeometryResult& g, bool long_path) {
    if (g.status != BearingStatus::Defined || std::isnan(g.short_bearing_deg))
        return path_summary("0 KM", "BEARING UNDEFINED");
    const double km = long_path ? g.long_distance_km : g.distance_km;
    const double deg = long_path ? g.long_bearing_deg : g.short_bearing_deg;
    return QString("%1 · %2 KM  ·  %3 DEG / TRUE")
        .arg(long_path ? "Long path" : "Short path")
        .arg(qRound(km)).arg(qRound(deg) % 360);
}

} // namespace hamclock
