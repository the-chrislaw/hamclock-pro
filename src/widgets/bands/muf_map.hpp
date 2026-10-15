#pragma once
// MUF map style: the 3000 km single-hop MUF over the globe from the in-house
// path model (approximate, not a measured map). QtGui only.
#include "../../domain/clock_state.hpp"
#include "../../map/styles.hpp"

namespace hamclock::bands {
ColorScale muf_scale();
// Equirectangular 1-degree image, north up.
QImage muf_image(double sfi, UtcSeconds utc);
}
