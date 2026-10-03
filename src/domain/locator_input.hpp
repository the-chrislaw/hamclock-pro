#pragma once
#include "geometry.hpp"
#include <string>

namespace hamclock {
// Original decoding to the locator's southwest corner (degrees north/east).
// Provisional replacement input policy: exactly 2, 4, or 6 ASCII characters,
// case-insensitive; fields A-R, optional squares 0-9, subsquares A-X.
// Whitespace, non-ASCII, malformed pairs, or other lengths throw invalid_argument.
// This does not establish the final original HamClock's supported input lengths.
Location location_from_maidenhead(const std::string& locator);
}
