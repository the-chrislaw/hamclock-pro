#include "dst.hpp"
#include "json_mini.hpp"
#include "swpc.hpp"
#include <algorithm>
#include <cmath>

namespace hcp::dst {

std::optional<std::vector<Sample>> parse(std::string_view body) {
    const auto doc = json::parse(body);
    if (!doc || !doc->is_array()) return std::nullopt;
    std::vector<Sample> out;
    for (const auto& rec : doc->array) {
        if (!rec.is_object()) continue;
        const auto& value = rec.at("dst");
        if (!value.is_number() || !std::isfinite(value.number)) continue; // null or text: no reading that hour
        const auto t = swpc::parse_iso_utc_ms(rec.at("time_tag").as_string());
        if (!t) continue;
        out.push_back({*t, static_cast<int>(std::lround(value.number))});
    }
    std::stable_sort(out.begin(), out.end(), [](const Sample& a, const Sample& b) { return a.time_ms < b.time_ms; });
    return out;
}

std::string word(int dst) {
    if (dst > -30) return "Quiet";
    if (dst > -50) return "Weak";
    if (dst > -100) return "Moderate";
    if (dst >= -250) return "Intense";
    return "Super";
}

} // namespace hcp::dst
