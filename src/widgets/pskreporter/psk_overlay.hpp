#pragma once
// Map overlay layer for PSKReporter receivers (see docs/map-overlays.md).
#include "../../map/overlay.hpp"
#include "psk_parse.hpp"
#include <vector>

namespace hamclock::psk {

inline constexpr char kOverlayLayerId[] = "psk";

// One marker per receiving station (newest report wins, input is newest
// first), placed at the centre of its 4- or 6-character grid square with a
// short path from DE. Receivers without a usable grid are not drawn.
OverlayLayer overlay_layer(const std::vector<Report>& reports, int max_markers = 30);

} // namespace hamclock::psk
