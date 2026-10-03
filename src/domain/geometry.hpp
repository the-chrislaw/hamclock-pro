#pragma once

#include <string>

namespace hamclock {

struct Location { double latitude, longitude; };
enum class BearingStatus { Defined, Coincident, Antipodal };
struct GeometryResult {
    double distance_km;
    double short_bearing_deg;
    double long_distance_km;
    double long_bearing_deg;
    BearingStatus status;
};

// Original spherical calculations using mean Earth radius 6371.0088 km.
// Locations use degrees north/east; +180 longitude is equivalent to -180.
// Bearings are initial true azimuths clockwise from north in [0,360).
// Long path is the complementary great-circle arc, not an ellipsoidal route.
// Coincident and antipodal endpoints have no unique bearing: both bearings
// are NaN. Points within 1e-12 radians of these degeneracies are classified
// likewise. These calculations describe geometry, not propagation forecasts.
// Nonfinite or out-of-range coordinates throw std::invalid_argument.
GeometryResult geometry(Location de, Location dx);
bool valid_location(Location location) noexcept;

// Maidenhead locators use upper-case fields/squares and lower-case subsquares.
// Precision is the total character count: 2, 4, or 6. Replacement boundary
// policy uses canonical representable SW corners: longitude i/12-180 and
// latitude i/24-90 for integral i in [0,4320]. An exact canonical corner enters
// its north/east cell; the immediately preceding representable double stays
// south/west. No epsilon or coordinate nudge is used. This policy matches the
// decoder's rational-double reconstruction, not a claim of original numeric
// implementation parity. +180 wraps west and +90 uses the last northern cell.
// Invalid coordinates or precision throw std::invalid_argument.
std::string maidenhead(Location location, unsigned precision = 6);

int run_domain_tests();

} // namespace hamclock
