#pragma once
// Map overlay layer for the stations in the ADIF log (see docs/map-overlays.md). A QSO is placed at the
// centre of its logged Maidenhead grid; QSOs without a usable grid are not drawn (no guessing from the call).
#include "../../map/overlay.hpp"
#include "adif_parse.hpp"
#include <vector>

namespace hamclock::adif {

inline constexpr char kOverlayLayerId[] = "adif";

// `qsos` newest first; one marker per distinct base call (newest QSO wins); at most `max_markers`.
OverlayLayer overlay_layer(const std::vector<Qso>& qsos, int max_markers = 200);

} // namespace hamclock::adif
