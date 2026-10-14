#pragma once
// Map overlay layer for earthquakes (see docs/map-overlays.md).
#include "../../map/overlay.hpp"
#include "quakes_parse.hpp"
#include <vector>

namespace hamclock::quakes {

inline constexpr char kOverlayLayerId[] = "quakes";

// One marker per quake, strongest first so the labels ("M6.8") of the biggest
// events win the label-spacing rule; at most `max_markers`. Size and colour
// grow with magnitude: yellow below M4.5, orange to M6, red from M6.
OverlayLayer overlay_layer(const std::vector<Quake>& quakes, int max_markers = 60);

} // namespace hamclock::quakes
