#include "scales.hpp"
#include "json_mini.hpp"
#include "text_util.hpp"

namespace hcp::scales {
namespace {
// SWPC sends numbers as strings, or null when a record has no value.
int number(const json::Value& v, int max) {
    std::optional<int> n;
    if (v.type == json::Value::Type::String) n = text::to_int(v.string);
    else if (v.is_number()) n = text::whole_or(v.number, -1);
    return n && *n >= 0 && *n <= max ? *n : -1;
}

Day parse_day(const json::Value& rec) {
    const auto& r = rec.at("R");
    const auto& s = rec.at("S");
    const auto& g = rec.at("G");
    Day d;
    d.r = number(r.at("Scale"), 5);
    d.s = number(s.at("Scale"), 5);
    d.g = number(g.at("Scale"), 5);
    d.r_minor = number(r.at("MinorProb"), 100);
    d.r_major = number(r.at("MajorProb"), 100);
    d.s_prob = number(s.at("Prob"), 100);
    return d;
}
} // namespace

Scales parse(std::string_view body) {
    Scales out;
    const auto doc = json::parse(body);
    if (!doc || !doc->is_object() || !doc->at("0").is_object()) return out;
    const auto& now = doc->at("0");
    out.current = parse_day(now);
    if (out.current.r < 0 || out.current.s < 0 || out.current.g < 0) return out; // no current state to show
    const auto when = text::parse_ymd(now.at("DateStamp").as_string(), now.at("TimeStamp").as_string());
    if (!when) return out;
    out.observed_ms = *when;
    if (doc->at("-1").is_object()) out.past24h = parse_day(doc->at("-1"));
    // Day 1 of the feed is the rest of today, 2 and 3 the following days.
    for (size_t i = 0; i < out.forecast.size(); ++i) {
        const auto& rec = doc->at(std::to_string(i + 1));
        if (rec.is_object()) out.forecast[i] = parse_day(rec);
    }
    out.valid = true;
    return out;
}

std::string scale_word(int level) {
    static const char* words[] = {"", "Minor", "Moderate", "Strong", "Severe", "Extreme"};
    return level >= 1 && level <= 5 ? words[level] : std::string();
}

} // namespace hcp::scales
