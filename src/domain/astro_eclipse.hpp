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
}
