#pragma once
// Qt-free helpers for the satellite pass surfaces (iPhone widgets, tvOS): the
// display short name used by the Qt app's [satellites] tracked= list, and the
// next passes of one TLE with AOS/LOS azimuths. short_name is a port of
// satellite_short_name in src/widgets/satellites/satellites_model.cpp, which is the oracle.
#include "domain/astro_satellite.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace hcp::sats {

// "ISS (ZARYA)" -> "ISS", "SAUDISAT 1C (SO-50)" -> "SO-50", "FUNCUBE-1 & ..." -> "FUNCUBE-1".
std::string short_name(const std::string& tle_name);

struct Pass {
    std::int64_t aos = 0, tca = 0, los = 0; // unix seconds; aos == from while the satellite is up
    double max_el_deg = 0, aos_az_deg = 0, los_az_deg = 0;
};

// Passes above min_el_deg after `from` within horizon_s (at most max_passes).
std::vector<Pass> next_passes(const hamclock::Tle& tle, const hamclock::Observer& obs, std::int64_t from,
                              std::int64_t horizon_s, double min_el_deg, int max_passes);

} // namespace hcp::sats
