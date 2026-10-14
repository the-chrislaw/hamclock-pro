#pragma once
#include "clock_state.hpp"
namespace hamclock {
// Offline positions of the five naked-eye planets from the JPL "Keplerian
// Elements for Approximate Positions of the Major Planets" (E.M. Standish,
// valid 1800-2050; see the Data source policy in docs/ROADMAP.md). Precessed to the date, no
// light-time or aberration: good to roughly 0.1-0.3 degree, ample for finding a
// planet in the sky and for a map marker, not for occultations.
enum class Planet { Mercury, Venus, Mars, Jupiter, Saturn };
constexpr int kPlanetCount = 5;
const char* planet_name(Planet p);
struct PlanetPosition {
    double ecliptic_lon_deg;  // geocentric, of date
    double ra_deg, dec_deg;   // geocentric equatorial of date
    double distance_au;       // from Earth
    double elongation_deg;    // angular distance from the Sun
    double sun_lon_deg;       // geocentric ecliptic longitude of the Sun, of date
};
PlanetPosition planet_position(Planet p, UtcSeconds utc);
struct PlanetHorizontal { double azimuth_deg, elevation_deg; }; // azimuth from north, clockwise
// Geocentric (no parallax), no refraction. Longitude east positive; out-of-range throws std::invalid_argument.
PlanetHorizontal planet_horizontal(Planet p, UtcSeconds utc, double latitude_deg, double longitude_deg);
// Sub-planet point: where the planet is at the zenith.
struct PlanetSubpoint { double latitude_deg, longitude_deg; };
PlanetSubpoint planet_subpoint(Planet p, UtcSeconds utc);
}
