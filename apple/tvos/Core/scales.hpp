#pragma once
// Qt-free port of src/widgets/solarscales/solar_scales_parse (NOAA products/noaa-scales.json).
// Same rules as the Qt parser, which stays the behavioural oracle in tests.
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace hcp::scales {

struct Day {
    int r = -1, s = -1, g = -1;      // scale 0-5 where SWPC states one, else -1
    int r_minor = -1, r_major = -1;  // forecast probability (%) of R1-R2 / R3+, -1 if absent
    int s_prob = -1;                 // forecast probability (%) of S1+
};

struct Scales {
    bool valid = false;
    int64_t observed_ms = 0;         // UTC, time of the "current" record
    Day current, past24h;
    std::array<Day, 3> forecast;     // today (rest of day), tomorrow, day after
};

// valid=false when the body is not the expected object or lacks the current record.
Scales parse(std::string_view body);
std::string scale_word(int level);   // "Minor".."Extreme"; empty for 0 or out of range

} // namespace hcp::scales
