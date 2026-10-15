#pragma once
// Qt-free port of src/widgets/quakes/quakes_parse (USGS GeoJSON summary feeds).
// Same rules as the Qt parser, which stays the behavioural oracle in tests.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::quakes {

struct Quake {
    double magnitude = 0;
    std::string place;
    int64_t time_ms = 0; // origin time, UTC
    double latitude = 0, longitude = 0, depth_km = 0;
};

// Rows with no numeric magnitude/time or bad coordinates are skipped; newest first.
// nullopt when the body is not a GeoJSON FeatureCollection.
std::optional<std::vector<Quake>> parse(std::string_view body);

// Greatest magnitude, earliest in the list on ties; nullptr if none.
const Quake* strongest(const std::vector<Quake>& quakes);

std::string magnitude_class(double magnitude); // Great / Major / Strong / Moderate / Light / Minor

} // namespace hcp::quakes
