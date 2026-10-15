#pragma once
#include "clock_state.hpp"
#include <optional>
namespace hamclock {
// Global solar eclipse circumstances after Meeus, Astronomical Algorithms 2nd
// ed. ch. 54 (time of greatest eclipse to a few minutes, gamma to about 0.005).
// Planetary perturbation terms of the new-moon time are omitted (under a minute).
// This says when and what kind of eclipse occurs somewhere on Earth, not whether
// it is visible at a given station.
enum class SolarEclipseType { Partial, Annular, Total, Hybrid };
struct SolarEclipse {
    UtcSeconds greatest;      // time of greatest eclipse, UTC
    SolarEclipseType type;
    double gamma;             // least distance of the shadow axis from Earth's centre, Earth radii (signed)
    double magnitude;         // partial eclipses only (fraction of the Sun's diameter); 0 otherwise
};
// First solar eclipse whose greatest eclipse is strictly after `after`; searches 30 lunations.
std::optional<SolarEclipse> next_solar_eclipse(UtcSeconds after);
const char* solar_eclipse_type_name(SolarEclipseType t);

// Whether the eclipse can be seen from a station, from the lunar ephemeris of
// astro_moon.hpp (about 0.1 degree) and the low-accuracy Sun. The window is
// +-4 h around the global greatest eclipse; only samples with the Sun above the
// horizon count. The discs touch at about 0.52 degrees separation, so under
// 0.40 is Visible, 0.40 to 0.64 Marginal (ephemeris error could decide it) and
// beyond that None. Also gives the local time of maximum and the magnitude there; contact times are not computed.
enum class EclipseVisibility { None, Marginal, Visible };
struct EclipseLocal {
    EclipseVisibility visibility;
    double min_separation_deg;   // smallest Sun-Moon separation while the Sun is up; 180 if never up
    double sun_elevation_deg;    // Sun elevation at that moment
    UtcSeconds max_time{0};      // moment of that smallest separation (local maximum, minute resolution); 0 if the Sun is never up
    double magnitude = 0;        // fraction of the Sun's diameter covered at that moment (>1 when total); 0 when None
};
// Latitude in [-90, 90] and longitude (east positive) in [-180, 180], else std::invalid_argument.
EclipseLocal eclipse_visibility(const SolarEclipse& e, double latitude_deg, double longitude_deg);
}
