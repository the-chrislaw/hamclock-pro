#pragma once
// Qt-free port of src/widgets/solaraurora/solar_aurora_parse (NOAA SWPC OVATION nowcast,
// json/ovation_aurora_latest.json). Parse and peaks only; the colour image stays in Swift.
#include <cstdint>
#include <string_view>
#include <vector>

namespace hcp::aurora {

constexpr int kLons = 360;
constexpr int kLats = 181;

struct Grid {
    int64_t observed_ms = 0, forecast_ms = 0;  // UTC; 0 when absent or unreadable
    std::vector<unsigned char> cells;          // kLons x kLats, index (lat + 90) * 360 + lon; missing cells 0
    bool valid = false;
    int at(int lat, int lon) const { return cells[static_cast<size_t>((lat + 90) * kLons + ((lon % kLons) + kLons) % kLons)]; }
};

// Invalid grid when the body is not the expected object or has no usable cells.
Grid parse(std::string_view body);
// Highest probability in the northern (lat >= 0) or southern hemisphere; 0 for an invalid grid.
int peak(const Grid& grid, bool north);

} // namespace hcp::aurora
