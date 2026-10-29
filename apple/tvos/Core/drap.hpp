#pragma once
// Qt-free port of src/widgets/solardrap/solar_drap_parse (NOAA SWPC D-Region Absorption Prediction,
// text/drap_global_frequencies.txt). Parse and peak only; the colour image stays in Swift.
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::drap {

constexpr int kCols = 90;  // longitude -178..178, step 4
constexpr int kRows = 90;  // latitude 89..-89, step 2 (north first)

struct Grid {
    int64_t valid_ms = 0;          // UTC, "Product Valid At"
    std::string xray, proton;      // SWPC messages, e.g. "Normal X-ray Background"
    std::vector<float> mhz;        // kRows x kCols, row 0 = latitude 89
    bool valid = false;
};

// Invalid grid unless the body has a valid time and kRows complete rows of kCols numbers.
Grid parse(std::string_view body);
double peak(const Grid& grid);    // highest frequency anywhere, MHz; 0 for an invalid grid

} // namespace hcp::drap
