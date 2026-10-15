#pragma once
// Applies the user's per-source label style and minimum label distance (Settings, options.ini
// [map_sources]) to a spot overlay layer. Defaults leave the layer's own values untouched.
#include "../../map/overlay.hpp"
#include "../../ui/app_settings.hpp"

namespace hamclock {

inline void apply_spot_label_style(OverlayLayer& layer, const AppSettings* settings, const QString& source) {
    if (!settings) return;
    const int mode = settings->mapSourceLabel(source);
    if (mode >= 0 && mode <= static_cast<int>(OverlayLabelMode::Dot)) layer.labels = static_cast<OverlayLabelMode>(mode);
    if (settings->mapMinLabelPx() > 0) layer.min_label_distance_px = settings->mapMinLabelPx();
}

} // namespace hamclock
