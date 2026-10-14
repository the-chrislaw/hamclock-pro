#pragma once
// Map overlay layer for DX cluster spots (see docs/map-overlays.md). Spots carry no position, so each
// is drawn at the centre of its DXCC entity (cty.dat), one marker per entity, newest spot first.
#include "../../map/overlay.hpp"
#include "dx_spot_parse.hpp"
#include <QString>
#include <deque>
#include <functional>
#include <optional>

namespace hamclock::dxcluster {

inline constexpr char kOverlayLayerId[] = "dx-cluster";

struct EntityPoint { double lat = 0, lon = 0; QString prefix; };
using EntityResolver = std::function<std::optional<EntityPoint>(const QString& call)>;

// Resolver backed by the bundled DXCC table.
EntityResolver bundled_entity_resolver();

// `spots` newest first. Spots whose call is unknown are skipped; at most `max_markers` markers.
OverlayLayer overlay_layer(const std::deque<DxSpot>& spots, const EntityResolver& resolve, int max_markers = 40);

} // namespace hamclock::dxcluster
