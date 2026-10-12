#pragma once
#include "clock_state.hpp"
namespace hamclock {
// Offline low-accuracy solar position (NOAA Solar Calculator algorithm,
// after Meeus, Astronomical Algorithms 2nd ed. ch. 25 and 28). Good to about
// 0.01 degrees for current dates; ample for a day/night map.
struct SubsolarPoint { double latitude_deg; double longitude_deg; };
// Point on Earth where the Sun is overhead. Longitude is in [-180, 180).
SubsolarPoint subsolar_point(UtcSeconds utc);
// Sun elevation above the horizon in degrees, geometric (no refraction).
// Invalid latitude/longitude throw std::invalid_argument.
double solar_elevation_deg(SubsolarPoint sun, double latitude_deg, double longitude_deg);
// Latitude of the day/night terminator (elevation 0) at a longitude, in
// [-90, 90]. Only one latitude per longitude exists away from the equinoxes.
double terminator_latitude_deg(SubsolarPoint sun, double longitude_deg);
}
