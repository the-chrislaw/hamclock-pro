#pragma once
// Map overlay layer for nearby APRS stations (see docs/map-overlays.md). Stations carry their own position.
#include "../../map/overlay.hpp"
#include "aprs_parse.hpp"
#include <vector>

namespace hamclock::aprs {

inline constexpr char kOverlayLayerId[] = "aprs";

// `stations` ordered most recently heard first; at most `max_markers` markers, non-finite positions skipped.
OverlayLayer overlay_layer(const std::vector<Station>& stations, int max_markers = 60);

} // namespace hamclock::aprs
