#pragma once
// Map overlay layer for tropical storms (see docs/map-overlays.md).
#include "../../map/overlay.hpp"
#include "storms_parse.hpp"
#include <vector>

namespace hamclock::storms {

inline constexpr char kOverlayLayerId[] = "storms";

// One marker per storm, strongest first, labelled with its name. Diamond size
// and colour grow with intensity: yellow tropical depression/storm, orange
// category 1-2, red category 3 and above. Forecast tracks are not drawn yet.
OverlayLayer overlay_layer(const std::vector<Storm>& storms);

} // namespace hamclock::storms
