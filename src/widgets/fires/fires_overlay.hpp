#pragma once
// Map overlay layer for active fires (see docs/map-overlays.md).
#include "../../map/overlay.hpp"
#include "fires_parse.hpp"
#include <vector>

namespace hamclock::fires {

inline constexpr char kOverlayLayerId[] = "fires";
inline constexpr size_t kMaxMarkers = 60;

// The strongest detections (low-confidence ones left out), no labels: orange
// below 50 MW, red above. Capped so a bad fire season cannot flood the map.
OverlayLayer overlay_layer(const std::vector<Detection>& detections);

} // namespace hamclock::fires
