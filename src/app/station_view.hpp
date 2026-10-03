#pragma once

#include "../platform/settings.hpp"
#include <string>

namespace hamclock {
struct StationView {
    std::string callsign;
    std::string de_grid;
    std::string dx_grid;
    std::string distance_text;
    std::string bearing_text;
    Location de;
    Location dx;
};

// Read-only presentation model. Does not load, save, or change settings.
// Values are uppercase ASCII for the current bitmap renderer.
StationView make_station_view(const StationSettings& settings);
int run_station_view_tests();
} // namespace hamclock
