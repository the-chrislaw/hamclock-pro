#include "satpass.hpp"

namespace hcp::sats {
namespace {
std::string trimmed(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}
} // namespace

std::string short_name(const std::string& tle_name) {
    const std::string name = trimmed(tle_name);
    const bool iss = name.find("ZARYA") != std::string::npos;
    const auto open = name.rfind('(');
    const auto close = name.rfind(')');
    if (open != std::string::npos && close != std::string::npos && close > open + 1 && !iss)
        return name.substr(open + 1, close - open - 1);
    if (iss) return "ISS";
    const auto amp = name.find(" &");
    return amp != std::string::npos && amp > 0 ? name.substr(0, amp) : name;
}

std::vector<Pass> next_passes(const hamclock::Tle& tle, const hamclock::Observer& obs, std::int64_t from,
                              std::int64_t horizon_s, double min_el_deg, int max_passes) {
    std::vector<Pass> out;
    if (max_passes <= 0 || horizon_s <= 0) return out;
    const hamclock::SatellitePropagator p(tle);
    if (!p.valid()) return out;
    const auto found = p.passes(obs, static_cast<double>(from), static_cast<double>(horizon_s), min_el_deg, max_passes);
    for (const auto& f : found) {
        Pass pass{f.aos, f.tca, f.los, f.max_el_deg, 0, 0};
        hamclock::LookAngle la;
        if (p.look(static_cast<double>(f.aos), obs, la)) pass.aos_az_deg = la.az_deg;
        if (p.look(static_cast<double>(f.los), obs, la)) pass.los_az_deg = la.az_deg;
        out.push_back(pass);
    }
    return out;
}

} // namespace hcp::sats
