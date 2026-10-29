#pragma once
// Qt-free port of src/widgets/fires/fires_parse (NASA FIRMS VIIRS active-fire CSV).
// Same rules as the Qt parser, which stays the behavioural oracle in tests.
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace hcp::fires {

struct Detection {
    double latitude = 0, longitude = 0;
    int64_t acquired_ms = 0;  // satellite overpass, UTC
    double frp_mw = 0;        // fire radiative power
    enum Confidence { Low, Nominal, High } confidence = Nominal;
};

// Strongest (FRP) first. nullopt without the FIRMS header columns; a header-only file is
// valid (no detections). Malformed rows are skipped.
std::optional<std::vector<Detection>> parse(std::string_view body);

} // namespace hcp::fires
