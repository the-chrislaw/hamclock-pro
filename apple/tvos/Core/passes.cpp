#include "passes.hpp"
#include "satpass.hpp"
#include "domain/astro_finder.hpp"
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <set>

namespace hcp::passes {
namespace {
constexpr double kHorizonS = 48 * 3600; // passes_model.cpp
constexpr double kMinEl = 10.0;
constexpr double kLookBackS = 900; // a pass in progress is still listed
constexpr int64_t kDayS = 86400;

int64_t utc_day(int64_t t) { return t >= 0 ? t / kDayS : (t - kDayS + 1) / kDayS; }

std::string two(int v) {
    char buf[4];
    std::snprintf(buf, sizeof buf, "%02d", v);
    return buf;
}

std::string utc_clock(int64_t t) {
    const int64_t s = t - utc_day(t) * kDayS;
    return two(static_cast<int>(s / 3600)) + ":" + two(static_cast<int>(s / 60 % 60)) + "Z";
}

std::string magnitude(const std::optional<double>& m) {
    if (!m) return {};
    char buf[16];
    std::snprintf(buf, sizeof buf, "%.1f", *m);
    return buf;
}

std::string empty_reason(Filter f, bool alerts_on) {
    switch (f) {
    case Filter::Visible: return "No passes visible to the eye in 48 h";
    case Filter::Alerts: return alerts_on ? "No alert-satellite passes in 48 h" : "Turn on satellite alerts in Settings";
    case Filter::All: break;
    }
    return "No passes in 48 h";
}
} // namespace

std::string day_label(int64_t unix_time, int64_t now) {
    const int64_t day = utc_day(unix_time), today = utc_day(now);
    if (day == today) return "Today";
    if (day == today + 1) return "Tomorrow";
    static const char* const kNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    // 1970-01-01 was a Thursday.
    return kNames[((day + 4) % 7 + 7) % 7];
}

std::vector<hamclock::Tle> fresh_tles(const std::vector<hamclock::Tle>& tles, double unix_time) {
    std::vector<hamclock::Tle> out;
    for (const auto& tle : tles) {
        const hamclock::SatellitePropagator p(tle);
        if (p.valid() && unix_time - p.epoch_unix() <= kMaxTleAgeDays * 86400.0) out.push_back(tle);
    }
    return out;
}

Result schedule(const std::vector<hamclock::Tle>& all, const hamclock::Observer& obs, int64_t now, Filter filter,
                const satalert::Prefs& alerts, const satalert::Catalog& catalog) {
    Result r;
    const double t = static_cast<double>(now);
    const auto tles = fresh_tles(all, t);
    if (tles.empty()) {
        r.state = all.empty() ? State::Loading : State::Stale;
        r.empty_reason = all.empty() ? "Waiting for satellite elements" : "Satellite elements are too old";
        return r;
    }
    std::vector<std::unique_ptr<hamclock::SatellitePropagator>> props;
    std::vector<std::pair<std::string, const hamclock::SatellitePropagator*>> sats;
    std::set<std::string> alert_names;
    for (const auto& tle : tles) {
        auto p = std::make_unique<hamclock::SatellitePropagator>(tle);
        if (!p->valid()) continue;
        const std::string name = hcp::sats::short_name(tle.name);
        if (alerts.enabled && satalert::is_selected(alerts, catalog, satalert::norad_id(tle))) alert_names.insert(name);
        sats.emplace_back(name, p.get());
        props.push_back(std::move(p));
    }
    std::function<bool(const std::string&)> include;
    if (filter == Filter::Alerts) include = [&](const std::string& n) { return alert_names.count(n) > 0; };
    const auto sched = hamclock::pass_schedule(sats, obs, t - kLookBackS, kHorizonS + kLookBackS, kMinEl,
                                               filter == Filter::Visible ? hamclock::PassFilter::VisibleToEye
                                                                         : hamclock::PassFilter::All,
                                               kMaxPasses * 2, include);
    for (const auto& s : sched) {
        if (s.pass.pass.los <= now) continue;
        Row row;
        row.name = s.name;
        row.aos = s.pass.pass.aos;
        row.los = s.pass.pass.los;
        row.day = day_label(row.aos, now);
        row.utc = utc_clock(row.aos);
        row.max_el_deg = static_cast<int>(std::lround(s.pass.pass.max_el_deg));
        row.from = s.pass.aos_compass;
        row.to = s.pass.los_compass;
        row.duration = std::to_string((s.pass.duration_s + 30) / 60) + " min";
        row.mag = magnitude(s.pass.brightest_mag);
        row.visible = s.pass.visible_to_eye;
        row.live = row.aos <= now;
        row.alert = alert_names.count(s.name) > 0;
        r.rows.push_back(std::move(row));
        if (static_cast<int>(r.rows.size()) >= kMaxPasses) break;
    }
    if (r.rows.empty()) r.empty_reason = empty_reason(filter, alerts.enabled);
    r.state = State::Live;
    return r;
}

} // namespace hcp::passes
