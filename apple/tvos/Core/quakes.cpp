#include "quakes.hpp"
#include "json_mini.hpp"
#include <algorithm>
#include <cmath>

namespace hcp::quakes {

std::optional<std::vector<Quake>> parse(std::string_view body) {
    const auto doc = json::parse(body);
    if (!doc || !doc->is_object()) return std::nullopt;
    const auto& features = doc->at("features");
    if (doc->at("type").as_string() != "FeatureCollection" || !features.is_array()) return std::nullopt;
    std::vector<Quake> out;
    for (const auto& item : features.array) {
        const auto& props = item.at("properties");
        const auto& coords = item.at("geometry").at("coordinates");
        const auto& mag = props.at("mag");
        const auto& time = props.at("time");
        if (!mag.is_number() || !time.is_number() || !coords.is_array() || coords.array.size() < 2 ||
            !coords.array[0].is_number() || !coords.array[1].is_number())
            continue;
        Quake q;
        q.magnitude = mag.number;
        q.longitude = coords.array[0].number;
        q.latitude = coords.array[1].number;
        q.depth_km = coords.array.size() > 2 && coords.array[2].is_number() ? coords.array[2].number : 0;
        if (!std::isfinite(q.magnitude) || std::fabs(q.latitude) > 90 || std::fabs(q.longitude) > 180) continue;
        q.time_ms = static_cast<int64_t>(time.number);
        q.place = props.at("place").as_string();
        out.push_back(std::move(q));
    }
    std::stable_sort(out.begin(), out.end(), [](const Quake& a, const Quake& b) { return a.time_ms > b.time_ms; });
    return out;
}

const Quake* strongest(const std::vector<Quake>& quakes) {
    const Quake* best = nullptr;
    for (const auto& q : quakes)
        if (!best || q.magnitude > best->magnitude) best = &q;
    return best;
}

std::string magnitude_class(double m) {
    if (!std::isfinite(m)) return {};
    if (m >= 8) return "Great";
    if (m >= 7) return "Major";
    if (m >= 6) return "Strong";
    if (m >= 5) return "Moderate";
    if (m >= 4) return "Light";
    return "Minor";
}

} // namespace hcp::quakes
