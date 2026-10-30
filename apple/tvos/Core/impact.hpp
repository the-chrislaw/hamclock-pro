#pragma once
// Qt-free port of src/widgets/solarimpact/solar_impact_model (the oracle): the NOAA R, S and G
// levels, plus the Kyoto Dst index while its latest hourly value is under 6 h old, ranked by
// operating impact (worst first), each with a one-line effect taken from the NOAA scale definitions.
#include "dst.hpp"
#include "scales.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace hcp::impact {

struct Entry {
    char letter = 'R'; // 'R', 'S', 'G' or 'D'
    int level = 0;     // 0-5 (-1 when SWPC states no level)
    std::string word;  // "Severe"; empty for level 0 on R/S/G; Dst storm word for D
    int max24 = 0;     // 24 h maximum level (Dst: level of the 24 h low)
    std::string effect;
};

// Effect text for a level (0-5) of 'R', 'S', 'G' or 'D'; empty if out of range.
std::string effect(char letter, int level);
// Impact level (0-5) for a Dst value in nT, comparable to the NOAA scales.
int dst_level(int dst);

// Worst first; ties: higher 24 h max, then R, S, G, D. Empty when scales are not valid.
std::vector<Entry> ranked(const scales::Scales& s, const std::vector<dst::Sample>& dst, int64_t now_ms);
// "All quiet" or "G3 Strong: <effect>"; empty when scales are not valid.
std::string headline(const scales::Scales& s, const std::vector<dst::Sample>& dst, int64_t now_ms);

} // namespace hcp::impact
