#pragma once
#include "clock_state.hpp"
#include <optional>
#include <vector>
namespace hamclock {
// Offline lunar ephemeris: the principal periodic terms of Meeus,
// Astronomical Algorithms 2nd ed. ch. 47 (about 0.1 degree in longitude and
// 0.05 degree in latitude, distance within a few hundred km), plus phase and
// topocentric rise/set. Ample for a dashboard and for EME planning; not for
// eclipse prediction. UTC is used as TT plus a fixed 69 s (delta-T).
struct MoonPosition {
    double longitude_deg, latitude_deg; // geocentric ecliptic of date
    double distance_km;
    double ra_deg, dec_deg;             // geocentric equatorial of date
};
MoonPosition moon_position(UtcSeconds utc);

struct MoonHorizontal { double azimuth_deg; double elevation_deg; }; // azimuth from north, clockwise
// Topocentric (parallax corrected, no refraction). Longitude east positive.
// Out-of-range latitude/longitude throw std::invalid_argument.
MoonHorizontal moon_horizontal(UtcSeconds utc, double latitude_deg, double longitude_deg);

struct MoonPhase {
    double elongation_deg;   // Moon minus Sun ecliptic longitude, [0, 360)
    double illuminated;      // 0..1
    bool waxing;
    const char* name;        // "New Moon", "Waxing Crescent", ... "Waning Crescent"
};
MoonPhase moon_phase(UtcSeconds utc);

enum class MoonEventKind { Rise, Set };
struct MoonEvent { MoonEventKind kind; UtcSeconds time; };
// Rise/set (upper limb, standard refraction) within [day_start, day_start+86400),
// sorted; day_start_utc must be a multiple of 86400. A day can have 0, 1 or 2.
std::vector<MoonEvent> moon_events_for_utc_day(std::int64_t day_start_utc, double latitude_deg, double longitude_deg);
// First event of the kind strictly after `after`, searching three days; empty if none.
std::optional<UtcSeconds> next_moon_event(UtcSeconds after, MoonEventKind kind, double latitude_deg, double longitude_deg);
// Meeus ch. 48: position angle of the Moon's bright limb from celestial north,
// eastward, in degrees [0, 360). Inputs are equatorial coordinates (degrees).
double bright_limb_position_angle(double sun_ra_deg, double sun_dec_deg, double moon_ra_deg, double moon_dec_deg);
// Direction of the bright limb as the observer sees it, counter-clockwise from
// straight up (zenith) in (-180, 180]: about -90 for a waxing Moon seen from
// the northern mid-latitudes (lit side right), +90 when waning. Accounts for
// the parallactic angle, so the disc can be drawn tilted as in the sky.
double moon_bright_limb_tilt_deg(UtcSeconds utc, double latitude_deg, double longitude_deg);
}
