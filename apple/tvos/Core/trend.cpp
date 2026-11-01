#include "trend.hpp"
#include "json_mini.hpp"
#include "text_util.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace hcp::trend {
namespace {
constexpr int64_t kDayMs = 86400 * 1000LL;
constexpr int64_t kWindowMs = 36 * 3600 * 1000LL;

const std::vector<json::Value>* rows(const std::optional<json::Value>& doc) {
    return doc && doc->is_array() ? &doc->array : nullptr;
}

// Qt: text.trimmed().remove('Z') parsed as ISO date (SWPC tags are UTC).
std::optional<int64_t> parse_time(std::string_view text) {
    std::string t;
    for (char c : text)
        if (c != 'Z') t.push_back(c);
    const auto first = t.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return std::nullopt;
    const auto last = t.find_last_not_of(" \t\r\n");
    return hcp::text::parse_iso8601_ms(std::string_view(t).substr(first, last - first + 1));
}

int64_t floor_day(int64_t ms) { return (ms >= 0 ? ms / kDayMs : (ms - kDayMs + 1) / kDayMs) * kDayMs; }
} // namespace

std::optional<std::vector<KpBar>> kp_bars(std::string_view body) {
    const auto doc = json::parse(body);
    const auto* list = rows(doc);
    if (!list) return std::nullopt;
    std::vector<KpBar> all;
    for (const auto& row : *list) {
        if (!row.is_object()) continue;
        const auto& kp = row.at("kp");
        const auto time = parse_time(row.at("time_tag").as_string());
        if (!time || !kp.is_number() || !(kp.number >= 0 && kp.number <= 9)) continue;
        all.push_back({*time, kp.number, row.at("observed").as_string() == "predicted"});
    }
    std::stable_sort(all.begin(), all.end(), [](const KpBar& a, const KpBar& b) { return a.time_ms < b.time_ms; });
    all.erase(std::unique(all.begin(), all.end(), [](const KpBar& a, const KpBar& b) { return a.time_ms == b.time_ms; }),
              all.end());
    std::optional<int64_t> newest_real;
    for (const auto& bar : all)
        if (!bar.predicted) newest_real = bar.time_ms;
    if (!newest_real) return std::vector<KpBar>{}; // a forecast alone is not an observation
    const int64_t start = *newest_real - kWindowMs;
    all.erase(all.begin(), std::find_if(all.begin(), all.end(), [&](const KpBar& b) { return b.time_ms >= start; }));
    return all;
}

std::optional<std::vector<FluxDay>> flux_daily(std::string_view body, int days) {
    const auto doc = json::parse(body);
    const auto* list = rows(doc);
    if (!list) return std::nullopt;
    std::map<int64_t, double> by_day;
    for (const auto& row : *list) {
        if (!row.is_object()) continue;
        const auto& flux = row.at("flux");
        const auto time = parse_time(row.at("time_tag").as_string());
        if (row.at("reporting_schedule").as_string() != "Noon" || !time || !flux.is_number()) continue;
        if (!(flux.number > 0 && flux.number <= 1000)) continue;
        by_day[floor_day(*time)] = flux.number;
    }
    std::vector<FluxDay> out;
    for (const auto& [day, flux] : by_day) out.push_back({day, flux});
    if (days > 0 && out.size() > static_cast<size_t>(days)) out.erase(out.begin(), out.end() - days);
    return out;
}

std::optional<Sunspots> latest_sunspots(std::string_view body) {
    const auto doc = json::parse(body);
    const auto* list = rows(doc);
    if (!list) return std::nullopt;
    std::string best_month;
    double best = 0;
    for (const auto& row : *list) {
        if (!row.is_object()) continue;
        const std::string month(row.at("time-tag").as_string());
        const auto& ssn = row.at("observed_swpc_ssn");
        if (month.size() != 7 || !ssn.is_number() || !(ssn.number >= 0)) continue;
        if (month > best_month) { // ISO months sort lexically
            best_month = month;
            best = ssn.number;
        }
    }
    if (best_month.empty()) return std::nullopt;
    return Sunspots{best_month, best};
}

int kp_level(double kp) { return kp >= 5 ? 2 : kp >= 4 ? 1 : 0; }

} // namespace hcp::trend
