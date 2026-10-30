#include "solarwind.hpp"
#include "json_mini.hpp"
#include "swpc.hpp"
#include <algorithm>
#include <cmath>

namespace hcp::solarwind {

std::vector<Point> series(std::string_view body, const std::string& field) {
    auto s = checked_series(body, field);
    return s ? std::move(*s) : std::vector<Point>{};
}

std::optional<std::vector<Point>> checked_series(std::string_view body, const std::string& field) {
    auto all = checked_series(body, std::vector<std::string>{field});
    if (!all) return std::nullopt;
    return std::move(all->front());
}

namespace {
// Oldest first, at most one point per timestamp (the first row wins, as before).
void sort_unique(std::vector<Point>& s) {
    std::stable_sort(s.begin(), s.end(), [](const Point& a, const Point& b) { return a.time_ms < b.time_ms; });
    s.erase(std::unique(s.begin(), s.end(), [](const Point& a, const Point& b) { return a.time_ms == b.time_ms; }),
            s.end());
}
} // namespace

std::optional<std::vector<std::vector<Point>>> checked_series(std::string_view body,
                                                              const std::vector<std::string>& fields) {
    const auto doc = json::parse(body);
    if (!doc || !doc->is_array()) return std::nullopt;
    std::vector<std::vector<Point>> out(fields.size());
    for (const auto& row : doc->array) {
        if (!row.is_object()) continue;
        const auto& active = row.at("active");
        if (active.type != json::Value::Type::Bool || !active.boolean) continue;
        const auto t = swpc::parse_iso_utc_ms(row.at("time_tag").as_string());
        if (!t) continue;
        for (size_t f = 0; f < fields.size(); ++f) {
            const auto& value = row.at(fields[f]);
            if (!value.is_number()) continue;
            const double v = value.number;
            if (!std::isfinite(v) || v <= -9999 + 1e-9) continue;
            out[f].push_back({*t, v});
        }
    }
    for (auto& s : out) sort_unique(s);
    return out;
}

std::optional<Point> latest(std::string_view body, const std::string& field) {
    const auto s = series(body, field);
    if (s.empty()) return std::nullopt;
    return s.back();
}

std::string speed_description(double km_s) {
    if (!std::isfinite(km_s) || km_s < 0) return {};
    if (km_s < 400) return "Slow";
    if (km_s < 600) return "Moderate";
    if (km_s < 800) return "Fast";
    return "Very fast";
}

std::string bz_description(double nt) {
    if (!std::isfinite(nt)) return {};
    if (nt > -3) return nt >= 0 ? "Northward" : "Near neutral";
    if (nt > -8) return "Weakly south";
    if (nt > -15) return "Southward";
    return "Strongly south";
}

} // namespace hcp::solarwind
