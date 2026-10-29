#pragma once
// Qt-free port of src/widgets/ota/ota_parse (POTA api.pota.app/spot/activator, SOTA
// api-db2.sota.org.uk spots, WWFF spots.wwff.co). Same rules as the Qt parsers: per feed,
// rows without call/reference/frequency/time and QRT spots are skipped, newest first.
// Like the Qt parse_* functions, duplicates are NOT collapsed here (that is ota::merge).
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::ota {

struct Spot {
    std::string program;     // "POTA", "SOTA" or "WWFF"
    std::string call;        // activator, upper case
    std::string reference;   // "US-1892" or "W4C/CM-009"
    std::string name;        // park or summit name, may be empty
    double freq_khz = 0;
    std::string mode;
    int64_t time_ms = 0;     // UTC
    double latitude = 0, longitude = 0;
    bool has_position = false;
    std::string comment;
};

// nullopt when the body is not a JSON array.
std::optional<std::vector<Spot>> parse_pota(std::string_view body);
std::optional<std::vector<Spot>> parse_sota(std::string_view body);  // MHz on the wire
std::optional<std::vector<Spot>> parse_wwff(std::string_view body);  // kHz, epoch spot_time

// DX cluster band table (src/widgets/dxcluster band_for_khz): "20m"; empty outside the bands.
std::string band_for_khz(double khz);

} // namespace hcp::ota
