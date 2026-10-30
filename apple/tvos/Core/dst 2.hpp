#pragma once
// Qt-free port of src/widgets/solardst/solar_dst_parse (Kyoto Dst via NOAA SWPC products/kyoto-dst.json).
// Same rules as the Qt parser, which stays the behavioural oracle in tests.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::dst {

struct Sample {
    int64_t time_ms = 0; // UTC, start of the hour
    int dst = 0;         // nT
};

// Oldest first; records with a missing time or value are skipped. nullopt when the body is not a JSON array.
std::optional<std::vector<Sample>> parse(std::string_view body);

// "Quiet" (> -30 nT), "Weak" (-30..-50), "Moderate" (-50..-100), "Intense" (-100..-250), "Super" (< -250).
std::string word(int dst);

} // namespace hcp::dst
