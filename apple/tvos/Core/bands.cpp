#include "bands.hpp"
#include <algorithm>
#include <cmath>

namespace hcp::bands {
namespace {
struct BandDef { const char* name; double mhz; };
constexpr BandDef kBands[] = {{"80m", 3.6}, {"40m", 7.1}, {"30m", 10.12}, {"20m", 14.2},
                              {"17m", 18.1}, {"15m", 21.2}, {"12m", 24.9}, {"10m", 28.4}};

Condition classify(double mhz, double muf) {
    if (mhz <= 0.85 * muf) return Condition::Good;
    if (mhz <= muf) return Condition::Fair;
    return Condition::Poor;
}
Condition lowered(Condition c, int steps) {
    return static_cast<Condition>(std::max(0, static_cast<int>(c) - steps));
}
} // namespace

std::vector<BandOutlook> hf_outlook(double sfi, double kp, double a_index) {
    sfi = std::isfinite(sfi) ? std::clamp(sfi, 60.0, 300.0) : 60.0;
    kp = std::isfinite(kp) ? kp : 0;
    a_index = std::isfinite(a_index) ? a_index : 0;
    const double day_muf = 15 + 0.15 * (sfi - 70);
    const double night_muf = std::max(6.0, 0.65 * day_muf);
    const int storm = (kp >= 7 || a_index >= 80) ? 2 : (kp >= 5 || a_index >= 30) ? 1 : 0;
    std::vector<BandOutlook> out;
    for (const auto& def : kBands) {
        BandOutlook b{def.name, def.mhz, classify(def.mhz, day_muf), classify(def.mhz, night_muf)};
        if (def.mhz < 5) b.day = Condition::Poor;
        else if (def.mhz < 8) b.day = std::min(b.day, Condition::Fair);
        b.day = lowered(b.day, storm);
        b.night = lowered(b.night, storm);
        out.push_back(b);
    }
    return out;
}

} // namespace hcp::bands
