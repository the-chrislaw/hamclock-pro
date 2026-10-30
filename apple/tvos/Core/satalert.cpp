#include "satalert.hpp"
#include "domain/astro_finder.hpp"
#include "json_mini.hpp"
#include "satpass.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>

namespace hcp::satalert {
namespace {
constexpr int kMaxNorad = 999999;
constexpr size_t kMaxEntries = 500;
constexpr double kMaxMhz = 11000; // QO-100 downlink is 10.49 GHz
constexpr int64_t kSecsPerMinute = 60;
constexpr double kHorizonS = 48 * 3600;
constexpr double kLookBackS = 3 * 3600;
constexpr size_t kMaxIds = 1000;

// A number (channel) or [low, high] (passband); nullopt when present but malformed.
std::optional<FreqRange> parse_freq(const json::Value& v) {
    if (v.type == json::Value::Type::Null) return FreqRange{}; // absent members read as null too
    FreqRange r;
    if (v.is_number()) {
        r = {v.number, v.number};
    } else if (v.is_array() && v.array.size() == 2 && v.array[0].is_number() && v.array[1].is_number()) {
        r = {v.array[0].number, v.array[1].number};
    } else {
        return std::nullopt;
    }
    if (!r.valid() || r.hi_mhz > kMaxMhz) return std::nullopt;
    return r;
}

std::optional<Transponder> parse_transponder(const json::Value& o) {
    static const std::array<std::string_view, 4> modes = {"FM", "Linear", "APRS", "Digital"};
    Transponder t;
    t.mode = std::string(o.at("mode").as_string());
    if (std::find(modes.begin(), modes.end(), t.mode) == modes.end()) return std::nullopt;
    const auto up = parse_freq(o.at("uplink_mhz")), down = parse_freq(o.at("downlink_mhz"));
    if (!up || !down || !down->valid()) return std::nullopt;
    t.uplink = *up;
    t.downlink = *down;
    t.ctcss_hz = o.at("ctcss_hz").is_number() ? o.at("ctcss_hz").number : 0;
    if (t.ctcss_hz < 0 || t.ctcss_hz > 300) return std::nullopt;
    return t;
}

std::string trim(std::string_view s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string_view::npos) return {};
    return std::string(s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1));
}

std::optional<Entry> parse_entry(const json::Value& o) {
    Entry e;
    const auto& n = o.at("norad");
    e.norad = n.is_number() && n.number == std::floor(n.number) && std::abs(n.number) < 1e9 ? static_cast<int>(n.number) : 0;
    e.name = trim(o.at("name").as_string());
    e.status = std::string(o.at("status").as_string());
    if (e.norad <= 0 || e.norad > kMaxNorad || e.name.empty()) return std::nullopt;
    if (e.status != "active" && e.status != "intermittent" && e.status != "inactive") return std::nullopt;
    if (o.at("transponders").is_array())
        for (const auto& tv : o.at("transponders").array) {
            const auto t = parse_transponder(tv);
            if (!t) return std::nullopt;
            e.transponders.push_back(*t);
        }
    if (e.transponders.empty()) return std::nullopt;
    return e;
}

std::vector<int> clean_ids(std::vector<int> ids) {
    ids.erase(std::remove_if(ids.begin(), ids.end(), [](int id) { return id <= 0; }), ids.end());
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    if (ids.size() > kMaxIds) ids.resize(kMaxIds);
    return ids;
}

bool contains(const std::vector<int>& ids, int id) { return std::find(ids.begin(), ids.end(), id) != ids.end(); }

std::vector<int> without(std::vector<int> ids, int id) {
    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
    return ids;
}

std::vector<int> with(std::vector<int> ids, int id) {
    ids.push_back(id);
    return clean_ids(std::move(ids));
}

std::string freq(const FreqRange& r) { // format_freq_range
    if (!r.valid()) return {};
    char buf[48];
    if (r.hi_mhz > r.lo_mhz) std::snprintf(buf, sizeof buf, "%.3f-%.3f", r.lo_mhz, r.hi_mhz);
    else std::snprintf(buf, sizeof buf, "%.3f", r.lo_mhz);
    return buf;
}

std::string hhmm(int64_t unix_s, int utc_offset_s) {
    const int64_t local = unix_s + utc_offset_s;
    const int64_t day_s = ((local % 86400) + 86400) % 86400;
    char buf[8];
    std::snprintf(buf, sizeof buf, "%02d:%02d", static_cast<int>(day_s / 3600), static_cast<int>(day_s % 3600 / 60));
    return buf;
}

int local_hour(int64_t unix_s, int utc_offset_s) {
    const int64_t local = unix_s + utc_offset_s;
    return static_cast<int>(((local % 86400) + 86400) % 86400 / 3600);
}

// Rounded up so "in 4 min" is never shown when 4 min 30 s remain.
int64_t minutes_until(int64_t from, int64_t to) { return (to - from + kSecsPerMinute - 1) / kSecsPerMinute; }
} // namespace

const Entry* Catalog::find(int norad) const {
    for (const auto& e : entries)
        if (e.norad == norad) return &e;
    return nullptr;
}

Catalog parse_catalog(std::string_view text) {
    Catalog out;
    const auto doc = json::parse(text);
    if (!doc || !doc->is_object()) return out;
    const auto& sats = doc->at("satellites");
    if (!sats.is_array()) return out;
    for (size_t i = 0; i < sats.array.size() && i < kMaxEntries; ++i) {
        const auto entry = parse_entry(sats.array[i]);
        if (!entry || out.find(entry->norad)) continue;
        out.entries.push_back(*entry);
    }
    return out;
}

Prefs sanitized(Prefs p) {
    p.min_elevation_deg = std::clamp(p.min_elevation_deg, 0, 90);
    p.lead_minutes = std::clamp(p.lead_minutes, 0, 60);
    const auto hour_ok = [](int h) { return h >= 0 && h <= 23; };
    if (!hour_ok(p.quiet_start_hour) || !hour_ok(p.quiet_end_hour) || p.quiet_start_hour == p.quiet_end_hour) {
        p.quiet_start_hour = -1;
        p.quiet_end_hour = -1;
    }
    p.muted = clean_ids(std::move(p.muted));
    p.picked = clean_ids(std::move(p.picked));
    return p;
}

bool is_selected(const Prefs& p, const Catalog& c, int norad) {
    if (norad <= 0 || contains(p.muted, norad)) return false;
    if (contains(p.picked, norad)) return true;
    const Entry* e = c.find(norad);
    return p.all_workable && e && e->workable();
}

Prefs toggle(const Prefs& p, const Catalog& c, int norad) {
    if (norad <= 0) return p;
    Prefs next = p;
    if (is_selected(p, c, norad)) {
        next.picked = without(p.picked, norad);
        if (is_selected(next, c, norad)) next.muted = with(p.muted, norad);
    } else {
        next.muted = without(p.muted, norad);
        if (!is_selected(next, c, norad)) next.picked = with(p.picked, norad);
    }
    return next;
}

bool in_quiet_hours(const Prefs& p, int h) {
    const int s = p.quiet_start_hour, e = p.quiet_end_hour;
    if (s < 0 || e < 0 || s == e || h < 0 || h > 23) return false;
    return s < e ? (h >= s && h < e) : (h >= s || h < e);
}

int norad_id(const hamclock::Tle& tle) {
    if (tle.line1.size() < 7) return 0;
    int n = 0;
    for (size_t i = 2; i < 7; ++i) {
        const char ch = tle.line1[i];
        if (ch == ' ') continue;
        if (ch < '0' || ch > '9') return 0;
        n = n * 10 + (ch - '0');
    }
    return n;
}

std::vector<AlertPass> predict(const std::vector<hamclock::Tle>& tles, const Prefs& p, const Catalog& c,
                               const hamclock::Observer& obs, int64_t now) {
    std::vector<std::unique_ptr<hamclock::SatellitePropagator>> props;
    std::map<int, std::string> names;
    std::vector<std::pair<std::string, const hamclock::SatellitePropagator*>> sats;
    for (const auto& tle : tles) {
        const int norad = norad_id(tle);
        if (!is_selected(p, c, norad) || names.count(norad)) continue;
        const Entry* e = c.find(norad);
        names[norad] = e ? e->name : hcp::sats::short_name(tle.name);
        props.push_back(std::make_unique<hamclock::SatellitePropagator>(tle));
        sats.emplace_back(std::to_string(norad), props.back().get());
    }
    std::vector<AlertPass> out;
    for (const auto& s : hamclock::pass_schedule(sats, obs, static_cast<double>(now) - kLookBackS, kHorizonS + kLookBackS,
                                                 0.0, hamclock::PassFilter::All, 0)) {
        if (static_cast<double>(s.pass.pass.aos) <= static_cast<double>(now) - kLookBackS + 1) continue;
        AlertPass a;
        a.norad = std::stoi(s.name);
        a.name = names[a.norad];
        a.aos = static_cast<int64_t>(s.pass.pass.aos);
        a.tca = static_cast<int64_t>(s.pass.pass.tca);
        a.los = static_cast<int64_t>(s.pass.pass.los);
        a.max_el_deg = s.pass.pass.max_el_deg;
        a.aos_compass = s.pass.aos_compass;
        a.los_compass = s.pass.los_compass;
        a.sun_el_deg = s.pass.sun_el_at_tca_deg;
        out.push_back(std::move(a));
    }
    return out;
}

std::vector<AlertPass> alert_passes(const std::vector<AlertPass>& passes, const Prefs& p, const Catalog& c,
                                    int utc_offset_s, int64_t now) {
    std::vector<AlertPass> out;
    if (!p.enabled) return out;
    for (const auto& a : passes) {
        if (a.los <= now || a.los <= a.aos) continue;
        if (!is_selected(p, c, a.norad)) continue;
        if (a.max_el_deg + 1e-9 < p.min_elevation_deg) continue;
        if (in_quiet_hours(p, local_hour(a.aos, utc_offset_s))) continue;
        if (p.daylight_only && a.sun_el_deg < 0) continue;
        out.push_back(a);
    }
    std::sort(out.begin(), out.end(), [](const AlertPass& x, const AlertPass& y) { return x.aos < y.aos; });
    return out;
}

std::string describe(const Transponder& t) {
    std::string text = t.mode + " " + freq(t.downlink) + " MHz";
    const bool simplex = t.uplink.valid() && freq(t.uplink) == freq(t.downlink);
    if (simplex) return text;
    text += " down";
    if (t.uplink.valid()) text += ", " + freq(t.uplink) + " up";
    if (t.ctcss_hz > 0) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "%.1f", t.ctcss_hz);
        text += std::string(", ") + buf + " Hz";
    }
    return text;
}

std::string alert_body(const AlertPass& pass, const Entry* entry, int utc_offset_s) {
    std::string text = "AOS " + hhmm(pass.aos, utc_offset_s) + " · max " +
                       std::to_string(static_cast<int>(std::lround(pass.max_el_deg))) + "°";
    if (!pass.aos_compass.empty() && !pass.los_compass.empty()) text += " · " + pass.aos_compass + " → " + pass.los_compass;
    if (entry && !entry->transponders.empty()) text += " · " + describe(entry->transponders.front());
    return text;
}

std::optional<Banner> current_banner(const std::vector<AlertPass>& passes, const Prefs& p, const Catalog& c,
                                     int utc_offset_s, int64_t now) {
    const int64_t lead = static_cast<int64_t>(p.lead_minutes) * kSecsPerMinute;
    for (const auto& a : alert_passes(passes, p, c, utc_offset_s, now)) {
        const Entry* e = c.find(a.norad);
        if (now < a.aos && now >= a.aos - lead)
            return Banner{false, a.name + " in " + std::to_string(minutes_until(now, a.aos)) + " min · " +
                                     alert_body(a, e, utc_offset_s)};
        if (now >= a.aos && p.announce_start) return Banner{true, a.name + " is up now · " + alert_body(a, e, utc_offset_s)};
    }
    return std::nullopt;
}

} // namespace hcp::satalert
