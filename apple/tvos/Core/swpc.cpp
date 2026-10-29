#include "swpc.hpp"
#include "json_mini.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>

namespace hcp::swpc {
namespace {
constexpr const char* kFlareBand = "0.1-0.8nm";

int64_t days_from_civil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = unsigned(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + int64_t(doe) - 719468;
}

std::optional<int64_t> row_time(const json::Value& row) {
    return parse_iso_utc_ms(row.at("time_tag").as_string());
}

std::vector<const json::Value*> rows(const std::optional<json::Value>& doc) {
    std::vector<const json::Value*> out;
    if (!doc || !doc->is_array()) return out;
    for (const auto& item : doc->array) if (item.is_object()) out.push_back(&item);
    return out;
}

template <class Read>
std::optional<Reading> latest(std::string_view body, Read read) {
    std::optional<Reading> best;
    const auto doc = json::parse(body);
    for (const auto* row : rows(doc)) {
        const auto time = row_time(*row);
        const auto value = read(*row);
        if (!time || !value || !std::isfinite(*value)) continue;
        if (!best || *time >= best->time_ms) best = Reading{*value, *time};
    }
    return best;
}

template <class Read>
std::optional<std::vector<Reading>> history(std::string_view body, Read read) {
    const auto doc = json::parse(body);
    if (!doc || !doc->is_array()) return std::nullopt;
    std::vector<Reading> out;
    for (const auto* row : rows(doc)) {
        const auto time = row_time(*row);
        const auto value = read(*row);
        if (time && value && std::isfinite(*value)) out.push_back(Reading{*value, *time});
    }
    std::stable_sort(out.begin(), out.end(), [](const Reading& a, const Reading& b) { return a.time_ms < b.time_ms; });
    return out;
}

std::optional<double> kp_value(const json::Value& row) {
    const auto& kp = row.at("Kp");
    if (!kp.is_number() || kp.number < 0 || kp.number > 9) return std::nullopt;
    return kp.number;
}

std::optional<double> xray_value(const json::Value& row) {
    const auto& f = row.at("flux");
    if (row.at("energy").as_string() != kFlareBand || !f.is_number() || f.number <= 0) return std::nullopt;
    return f.number;
}

std::string format(const char* fmt, char letter, double v, int digits) {
    char buf[32];
    std::snprintf(buf, sizeof buf, fmt, letter, digits, v);
    return buf;
}
} // namespace

std::optional<int64_t> parse_iso_utc_ms(std::string_view text) {
    while (!text.empty() && text.front() == ' ') text.remove_prefix(1);
    while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
    if (text.empty()) return std::nullopt;
    const std::string t(text);
    int Y, M, D, h, m, s, n = 0;
    if (std::sscanf(t.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d%n", &Y, &M, &D, &h, &m, &s, &n) != 6 || n != 19)
        return std::nullopt;
    if (M < 1 || M > 12 || D < 1 || D > 31 || h > 23 || m > 59 || s > 60) return std::nullopt;
    int64_t ms = 0;
    size_t i = 19;
    if (i < t.size() && t[i] == '.') {
        int scale = 100;
        for (++i; i < t.size() && t[i] >= '0' && t[i] <= '9'; ++i, scale /= 10) ms += scale * (t[i] - '0');
    }
    if (i < t.size() && t[i] == 'Z') ++i;
    if (i != t.size()) return std::nullopt;
    return ((days_from_civil(Y, unsigned(M), unsigned(D)) * 24 + h) * 60 + m) * 60 * 1000 + s * 1000 + ms;
}

std::optional<Reading> latest_kp(std::string_view body) { return latest(body, kp_value); }
std::optional<std::vector<Reading>> kp_history(std::string_view body) { return history(body, kp_value); }
std::optional<std::vector<Reading>> xray_history(std::string_view body) { return history(body, xray_value); }

std::optional<Reading> latest_solar_flux(std::string_view body) {
    return latest(body, [](const json::Value& row) -> std::optional<double> {
        const auto& f = row.at("flux");
        if (!f.is_number() || f.number <= 0 || f.number > 1000) return std::nullopt;
        return f.number;
    });
}

std::optional<Reading> latest_xray_flux(std::string_view body) { return latest(body, xray_value); }

std::vector<Reading> xray_series(std::string_view body, int bucket_minutes) {
    std::vector<Reading> out;
    if (bucket_minutes < 1) return out;
    const int64_t bucket_ms = int64_t(bucket_minutes) * 60 * 1000;
    std::map<int64_t, double> buckets;
    const auto doc = json::parse(body);
    for (const auto* row : rows(doc)) {
        const auto& f = row->at("flux");
        const auto time = row_time(*row);
        if (row->at("energy").as_string() != kFlareBand || !f.is_number() || !time || f.number <= 0) continue;
        const int64_t key = *time / bucket_ms;
        auto it = buckets.find(key);
        if (it == buckets.end()) buckets.emplace(key, f.number);
        else it->second = std::max(it->second, f.number);
    }
    for (const auto& [key, value] : buckets) out.push_back({value, key * bucket_ms});
    return out;
}

double xray_plot_fraction(double flux) {
    if (!std::isfinite(flux) || flux <= 1e-8) return 0;
    return std::min(1.0, (std::log10(flux) + 8) / 5);
}

std::string xray_class(double flux) {
    if (!std::isfinite(flux) || flux <= 0) return {};
    struct Band { char letter; double base; };
    static constexpr Band bands[] = {{'X', 1e-4}, {'M', 1e-5}, {'C', 1e-6}, {'B', 1e-7}, {'A', 1e-8}};
    for (const auto& band : bands) {
        if (flux >= band.base || band.letter == 'A') {
            constexpr double kEpsilon = 1e-9; // absorbs binary rounding; digits truncate per GOES
            const double scaled = flux / band.base;
            if (band.letter == 'X' && scaled >= 10) return format("%c%.*f", band.letter, std::floor(scaled + kEpsilon), 0);
            return format("%c%.*f", band.letter, std::floor(scaled * 10 + kEpsilon) / 10, 1);
        }
    }
    return {};
}

std::string kp_description(double kp) {
    if (!std::isfinite(kp)) return {};
    if (kp >= 9) return "Extreme storm (G5)";
    if (kp >= 8) return "Severe storm (G4)";
    if (kp >= 7) return "Strong storm (G3)";
    if (kp >= 6) return "Moderate storm (G2)";
    if (kp >= 5) return "Minor storm (G1)";
    if (kp >= 4) return "Active";
    return "Quiet";
}

} // namespace hcp::swpc
