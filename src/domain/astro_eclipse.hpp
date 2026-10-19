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
// beyond that None. Also gives the local time of maximum and the magnitude there; first and last contact times (20 s steps) are given when the Sun is partly covered.
enum class EclipseVisibility { None, Marginal, Visible };
struct EclipseLocal {
    EclipseVisibility visibility;
    double min_separation_deg;   // smallest Sun-Moon separation while the Sun is up; 180 if never up
    double sun_elevation_deg;    // Sun elevation at that moment
    UtcSeconds max_time{0};      // moment of that smallest separation (local maximum, minute resolution); 0 if the Sun is never up
    UtcSeconds start_time{0};    // first contact (or the last sample before sunrise when start_at_sunrise); 0 if no overlap
    UtcSeconds end_time{0};      // last contact (or the last sample before sunset when end_at_sunset)
    bool start_at_sunrise = false;
    bool end_at_sunset = false;
    double magnitude = 0;        // fraction of the Sun's diameter covered at that moment (>1 when total); 0 when None
};
// Latitude in [-90, 90] and longitude (east positive) in [-180, 180], else std::invalid_argument.
EclipseLocal eclipse_visibility(const SolarEclipse& e, double latitude_deg, double longitude_deg);

// Contact times at a station from the truncated ELP2000-82B Moon (astro_elp.hpp),
// the Meeus ch. 25 Sun (about 0.01 degree, which limits contact times to tens of
// seconds) and a fixed 69 s delta-T. C1/C4 are the outer contacts (Moon 1737.4 km),
// C2/C3 the inner ones (umbral radius 1736.5 km, as NASA uses). The horizon is
// not applied: check `sun_up_at_maximum` and the contact elevations yourself.
struct EclipseContacts {
    bool partial = false;          // discs overlap at all
    bool central = false;          // station inside the umbra/antumbra
    bool total = false;            // central and the Moon is the larger disc
    bool sun_up_at_maximum = false;
    UtcSeconds first{0}, second{0}, third{0}, last{0}; // C1..C4 (second/third only when central)
    UtcSeconds maximum{0};
    double min_separation_deg = 180.0;
    double magnitude = 0;          // fraction of the Sun's diameter covered at maximum
};
EclipseContacts eclipse_contacts(const SolarEclipse& e, double latitude_deg, double longitude_deg);
}
