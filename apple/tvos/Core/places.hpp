#pragma once
// Place search for first-run setup: port of src/ui/location_tools.cpp parseSearchReply
// (Nominatim jsonv2) and the Open-Meteo "timezone" field.
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::places {

struct Place {
    std::string name;   // display_name
    double lat = 0, lon = 0;
};

// nullopt when the body is not a JSON array; rows without string lat/lon in range are skipped.
std::optional<std::vector<Place>> parse_nominatim(std::string_view body);
// IANA zone of an Open-Meteo forecast reply ("Europe/Paris"); empty when absent.
std::string open_meteo_timezone(std::string_view body);

} // namespace hcp::places
