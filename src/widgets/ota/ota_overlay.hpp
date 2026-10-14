#pragma once
// Map overlay layer for On-The-Air activator spots (see docs/map-overlays.md).
#include "../../map/overlay.hpp"
#include "ota_parse.hpp"
#include <vector>

namespace hamclock::ota {

inline constexpr char kOverlayLayerId[] = "ota";

// Only spots with a real position are drawn (POTA carries the park's
// coordinates, SOTA spots do not). Input order is kept (newest first), at most
// `max_markers` markers. POTA = green dot, SOTA = orange triangle.
OverlayLayer overlay_layer(const std::vector<Spot>& spots, int max_markers = 40);

} // namespace hamclock::ota
