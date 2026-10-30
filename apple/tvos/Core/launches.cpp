#include "launches.hpp"
#include "json_mini.hpp"
#include "text_util.hpp"
#include <algorithm>

namespace hcp::launches {

std::optional<std::vector<Launch>> parse(std::string_view body) {
    const auto doc = json::parse(body);
    if (!doc || !doc->is_object() || !doc->at("results").is_array()) return std::nullopt;
    std::vector<Launch> out;
    for (const auto& o : doc->at("results").array) {
        Launch l;
        l.name = o.at("name").as_string();
        const auto net = text::parse_iso8601_ms(o.at("net").as_string());
        if (l.name.empty() || !net) continue;
        l.net_ms = *net;
        l.provider = o.at("lsp_name").as_string();
        l.mission = o.at("mission").as_string();
        l.mission_type = o.at("mission_type").as_string();
        l.pad = o.at("pad").as_string();
        l.location = o.at("location").as_string();
        l.status = o.at("status").at("abbrev").as_string();
        const auto prec = o.at("net_precision").at("abbrev").as_string();
        l.precise = prec == "MIN" || prec == "SEC" || prec == "HR";
        out.push_back(std::move(l));
    }
    std::stable_sort(out.begin(), out.end(), [](const Launch& a, const Launch& b) { return a.net_ms < b.net_ms; });
    return out;
}

std::string ham_hint(const Launch& l) {
    const std::string blob = l.name + ' ' + l.mission + ' ' + l.mission_type;
    const auto has = [&](const char* w) { return text::contains_ci(blob, w); };
    if (has("amateur") || has("oscar") || has("cubesat")) return "Amateur/cubesat payloads";
    if (has("rideshare") || has("transporter") || has("bandwagon")) return "Rideshare: possible amateur sats";
    if (has("international space station") || has("iss ") || has("crew-") || has("cygnus") || has("progress ms") ||
        has("soyuz ms") || has("CRS-"))
        return "ISS visit (ARISS)";
    return {};
}

std::optional<PadPosition> pad_position(const Launch& l) {
    struct Site { const char* key; double lat; double lon; };
    static const Site sites[] = {
        {"Cape Canaveral", 28.49, -80.57}, {"Kennedy Space Center", 28.57, -80.65},
        {"Vandenberg", 34.73, -120.57},    {"Starbase", 25.99, -97.15},
        {"Boca Chica", 25.99, -97.15},     {"Wallops", 37.94, -75.47},
        {"Mahia", -39.26, 177.86},         {"Kodiak", 57.44, -152.34},
        {"Kourou", 5.24, -52.77},          {"Baikonur", 45.96, 63.31},
        {"Plesetsk", 62.93, 40.58},        {"Vostochny", 51.88, 128.33},
        {"Jiuquan", 40.96, 100.28},        {"Taiyuan", 38.85, 111.61},
        {"Xichang", 28.25, 102.03},        {"Wenchang", 19.61, 110.95},
        {"Tanegashima", 30.40, 130.97},    {"Uchinoura", 31.25, 131.08},
        {"Satish Dhawan", 13.73, 80.23},   {"Sriharikota", 13.73, 80.23},
        {"Naro", 34.43, 127.54},           {"Semnan", 35.23, 53.92},
        {"Andoya", 69.29, 16.02},          {"Esrange", 67.89, 21.10},
    };
    for (const auto& s : sites)
        if (text::contains_ci(l.location, s.key)) return PadPosition{s.lat, s.lon};
    return std::nullopt;
}

std::string countdown_text(int64_t net_ms, int64_t now_ms) {
    const int64_t secs = (net_ms - now_ms) / 1000; // QDateTime::secsTo truncates
    if (secs <= 0) return "T+0m";
    const int64_t mins = secs / 60;
    if (mins >= 2880) return "T-" + std::to_string(mins / 1440) + "d " + std::to_string(mins % 1440 / 60) + "h";
    if (mins >= 60) return "T-" + std::to_string(mins / 60) + "h " + std::to_string(mins % 60) + "m";
    return "T-" + std::to_string(mins) + "m";
}

} // namespace hcp::launches
