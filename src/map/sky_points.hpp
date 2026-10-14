#pragma once
// Sky points on the map (QtCore-free): the sublunar point (where the Moon is
// overhead) and the antipode of a location.
#include "../domain/astro_moon.hpp"
#include "../domain/geometry.hpp"
#include <cmath>
namespace hamclock {
struct SkyPoint { double latitude_deg, longitude_deg; };
// Greenwich mean sidereal time in degrees [0, 360) (Meeus ch. 12).
inline double greenwich_sidereal_deg(UtcSeconds utc) {
    const double jd = static_cast<double>(utc.value) / 86400.0 + 2440587.5;
    return std::fmod(std::fmod(280.46061837 + 360.98564736629 * (jd - 2451545.0), 360.0) + 360.0, 360.0);
}
// Longitude in [-180, 180). The Moon's equatorial position comes from moon_position.
inline SkyPoint sublunar_point(UtcSeconds utc) {
    const auto m = moon_position(utc);
    return {m.dec_deg, std::remainder(m.ra_deg - greenwich_sidereal_deg(utc), 360.0)};
}
inline Location antipode(Location l) {
    double lon = std::remainder(l.longitude + 180.0, 360.0);
    if (lon >= 180.0) lon -= 360.0; // remainder can return +180; keep [-180, 180)
    return {-l.latitude, lon};
}
}
