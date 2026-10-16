#pragma once
#include "astro_moon.hpp"
#include <optional>
namespace hamclock {
// Earth-Moon-Earth planning helpers built on the offline lunar ephemeris.
// Geometric mutual visibility only (Moon above the horizon at both stations,
// no refraction, no horizon obstructions, no libration or polarisation).

// Mean lunar distance extremes used for the path-loss reference.
constexpr double kMoonPerigeeKm = 356400.0;  // closest approach of the orbit
constexpr double kMoonApogeeKm = 406700.0;

// Extra EME path loss (dB) relative to the Moon at perigee: 40 log10(d/d_perigee)
// (round trip, so the inverse fourth power of distance). Never negative.
double eme_degradation_db(double distance_km);

// Geocentric angular separation of the Moon from the Sun, degrees [0, 180].
// Near 0 (new Moon) the Sun sits behind the Moon and raises receiver noise.
double moon_sun_separation_deg(UtcSeconds utc);

// Next lunar perigee (or apogee) strictly after `after`, within 32 days: the
// minimum (maximum) of the ephemeris distance, found to about a minute. The
// ephemeris distance is good to a few hundred km, so the time is good to
// roughly an hour near the extreme (the distance curve is flat there).
struct LunarApsis {
    UtcSeconds time;
    double distance_km;
};
std::optional<LunarApsis> next_lunar_apsis(UtcSeconds after, bool perigee);

struct MutualWindow {
    UtcSeconds start;  // Moon already above both horizons: start == the search start
    UtcSeconds end;
    bool in_progress;  // the window began at or before `after`
};
// The first window at or after `after` (within `horizon_hours`, default 48)
// in which the Moon is above the horizon at both stations; edges are bisected
// to about a second. Empty when there is none. Longitude east positive.
std::optional<MutualWindow> next_mutual_moon_window(UtcSeconds after,
    double lat1, double lon1, double lat2, double lon2, int horizon_hours = 48);
}
