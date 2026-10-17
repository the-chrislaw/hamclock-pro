#pragma once
// Map overlay layer for Meshtastic nodes heard by the user's own node (see docs/map-overlays.md).
#include "../../map/overlay.hpp"
#include "mesh_parse.hpp"
#include <vector>

namespace hamclock::mesh {

inline constexpr char kOverlayLayerId[] = "mesh";

// `nodes` most recently heard first; at most `max_markers`; labels use the short name.
OverlayLayer overlay_layer(const std::vector<Node>& nodes, int max_markers = 60);

} // namespace hamclock::mesh
