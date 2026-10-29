#include "ota.hpp"
#include "json_mini.hpp"
#include "text_util.hpp"
#include <algorithm>
#include <cmath>

namespace hcp::ota {
namespace {

// Field names of one feed.
struct Fields {
    const char* program;
    const char* call;
    const char* reference;
    const char* name;
    const char* frequency;
    double freq_scale;       // to kHz
    const char* time;
    bool epoch_time;         // seconds since the epoch instead of an ISO string
    const char* comment;
};

double number(const json::Value& v) {
    if (v.is_number()) return v.number;
    if (v.type == json::Value::Type::String) return text::to_double(v.string).value_or(0);
    return 0;
}

void set_position(Spot& s, const json::Value& o) {
    const auto& lat = o.at("latitude");
    const auto& lon = o.at("longitude");
    if (!lat.is_number() || !lon.is_number()) return;
    s.latitude = lat.number;
    s.longitude = lon.number;
    s.has_position = std::fabs(s.latitude) <= 90 && std::fabs(s.longitude) <= 180 &&
                     !(s.latitude == 0 && s.longitude == 0);
}

std::optional<int64_t> spot_time(const json::Value& v, bool epoch) {
    if (!epoch) return text::parse_iso8601_ms(v.as_string());
    const double secs = number(v);
    constexpr double kMaxEpochSecs = 1e13; // far beyond any real spot; keeps the cast defined
    if (!(secs > 0) || secs > kMaxEpochSecs) return std::nullopt;
    return static_cast<int64_t>(secs) * 1000;
}

std::optional<std::vector<Spot>> parse_feed(std::string_view body, const Fields& f) {
    const auto doc = json::parse(body);
    if (!doc || !doc->is_array()) return std::nullopt;
    std::vector<Spot> out;
    for (const auto& o : doc->array) {
        Spot s;
        s.program = f.program;
        s.call = text::upper(text::trimmed(o.at(f.call).as_string()));
        s.reference = text::trimmed(o.at(f.reference).as_string());
        s.name = o.at(f.name).as_string();
        s.freq_khz = number(o.at(f.frequency)) * f.freq_scale;
        s.mode = text::upper(o.at("mode").as_string());
        const auto t = spot_time(o.at(f.time), f.epoch_time);
        s.comment = o.at(f.comment).as_string();
        set_position(s, o);
        if (s.call.empty() || s.reference.empty() || !(s.freq_khz > 0) || !t || text::contains_ci(s.comment, "qrt"))
            continue;
        s.time_ms = *t;
        out.push_back(std::move(s));
    }
    std::stable_sort(out.begin(), out.end(), [](const Spot& a, const Spot& b) { return a.time_ms > b.time_ms; });
    return out;
}

} // namespace

std::optional<std::vector<Spot>> parse_pota(std::string_view body) {
    return parse_feed(body, {"POTA", "activator", "reference", "name", "frequency", 1.0, "spotTime", false, "comments"});
}

std::optional<std::vector<Spot>> parse_sota(std::string_view body) {
    return parse_feed(body, {"SOTA", "activatorCallsign", "summitCode", "summitName", "frequency", 1000.0, "timeStamp",
                             false, "comments"});
}

std::optional<std::vector<Spot>> parse_wwff(std::string_view body) {
    return parse_feed(body, {"WWFF", "activator", "reference", "reference_name", "frequency_khz", 1.0, "spot_time",
                             true, "remarks"});
}

std::string band_for_khz(double khz) {
    static const struct { double lo, hi; const char* name; } bands[] = {
        {1800, 2000, "160m"}, {3500, 4000, "80m"}, {5250, 5450, "60m"}, {7000, 7300, "40m"},
        {10100, 10150, "30m"}, {14000, 14350, "20m"}, {18068, 18168, "17m"}, {21000, 21450, "15m"},
        {24890, 24990, "12m"}, {28000, 29700, "10m"}, {50000, 54000, "6m"}, {144000, 148000, "2m"},
        {430000, 450000, "70cm"}};
    for (const auto& b : bands)
        if (khz >= b.lo && khz <= b.hi) return b.name;
    return {};
}

} // namespace hcp::ota
