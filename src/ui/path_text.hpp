#pragma once

#include <QString>

namespace hamclock {

// DE == DX gives "0 KM" and "BEARING UNDEFINED", which reads as broken.
// Show a prompt instead; any other distance/bearing pair is shown as is.
inline QString path_summary(const QString& distance, const QString& bearing) {
    if (bearing.contains("UNDEFINED", Qt::CaseInsensitive)) return "Set a DX target in Settings";
    return distance + "  ·  " + bearing;
}

} // namespace hamclock
