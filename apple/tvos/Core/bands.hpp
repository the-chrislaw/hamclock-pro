#pragma once
// Qt-free port of src/widgets/bands/band_outlook (hf_outlook). Same model; the Qt
// regression expectations are the oracle. Rough heuristic, NOT VOACAP, not path-specific.
#include <vector>

namespace hcp::bands {

enum class Condition { Poor = 0, Fair = 1, Good = 2 };
struct BandOutlook {
    const char* band; // "20m"
    double mhz;
    Condition day;
    Condition night;
};

// Non-finite inputs fall back like the Qt code (SFI -> 60, Kp and A -> 0).
std::vector<BandOutlook> hf_outlook(double sfi, double kp, double a_index);

} // namespace hcp::bands
