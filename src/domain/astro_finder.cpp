#include "astro_finder.hpp"
#include "solar_position.hpp"
#include <algorithm>
#include <cmath>

namespace hamclock {
namespace {
double sun_elevation(const Observer& obs, double t) {
    return solar_elevation_deg(subsolar_point(UtcSeconds{static_cast<std::int64_t>(std::llround(t))}),
                               obs.lat_deg, obs.lon_deg);
}
double azimuth_at(const SatellitePropagator& sat, const Observer& obs, double t) {
    LookAngle la;
    return sat.look(t, obs, la) ? la.az_deg : 0.0;
}
} // namespace

std::string compass_letters(double az_deg) {
    static const char* const kNames[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                         "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    const double a = std::fmod(std::fmod(az_deg, 360.0) + 360.0, 360.0);
    return kNames[static_cast<int>(std::floor(a / 22.5 + 0.5)) % 16];
}

std::optional<PointingState> pointing_at(const SatellitePropagator& sat, const Observer& obs, double t) {
    LookAngle la;
    if (!sat.look(t, obs, la)) return std::nullopt;
    PointingState p;
    p.t = t;
    p.az_deg = la.az_deg;
    p.el_deg = la.el_deg;
    p.range_km = la.range_km;
    p.range_rate_km_s = la.range_rate_km_s;
    p.above_horizon = la.el_deg >= 0;
    if (!sat.sunlit(t, p.sunlit)) p.sunlit = false;
    return p;
}

DopplerPair doppler_for(const PointingState& p, double downlink_hz, double uplink_hz) {
    return {doppler_downlink_hz(downlink_hz, p.range_rate_km_s), doppler_uplink_hz(uplink_hz, p.range_rate_km_s)};
}

FinderPass describe_pass(const SatellitePropagator& sat, const Observer& obs, const SatPass& pass) {
    FinderPass f;
    f.pass = pass;
    f.duration_s = pass.los - pass.aos;
    f.aos_az_deg = azimuth_at(sat, obs, static_cast<double>(pass.aos));
    f.tca_az_deg = azimuth_at(sat, obs, static_cast<double>(pass.tca));
    f.los_az_deg = azimuth_at(sat, obs, static_cast<double>(pass.los));
    f.aos_compass = compass_letters(f.aos_az_deg);
    f.tca_compass = compass_letters(f.tca_az_deg);
    f.los_compass = compass_letters(f.los_az_deg);
    f.sun_el_at_tca_deg = sun_elevation(obs, static_cast<double>(pass.tca));
    for (double t = static_cast<double>(pass.aos); t <= static_cast<double>(pass.los) + 0.5; t += 15.0) {
        const auto p = pointing_at(sat, obs, t);
        if (p && p->el_deg > 0 && p->sunlit && sun_elevation(obs, t) < -6.0) { f.visible_to_eye = true; break; }
    }
    return f;
}

std::optional<double> estimate_magnitude(const SatellitePropagator& sat, const Observer& obs, double t, double std_mag) {
    const auto p = pointing_at(sat, obs, t);
    if (!p || p->el_deg <= 0 || !p->sunlit || sun_elevation(obs, t) >= -6.0) return std::nullopt;
    double ph = 0;
    if (!sat.phase_angle_deg(t, obs, ph)) return std::nullopt;
    const double r = ph * M_PI / 180.0;
    const double f = std::sin(r) + (M_PI - r) * std::cos(r);
    if (f <= 0) return std::nullopt;
    return std_mag - 15.0 + 5.0 * std::log10(p->range_km) - 2.5 * std::log10(f);
}

std::optional<double> standard_magnitude(const std::string& name) {
    if (name.rfind("ISS", 0) == 0) return -1.8;
    if (name.rfind("CSS", 0) == 0 || name.find("TIANHE") != std::string::npos) return -1.5;
    return std::nullopt;
}

std::optional<RisePoint> rise_point(const SatellitePropagator& sat, const Observer& obs, double after, double horizon_s) {
    const auto passes = sat.passes(obs, after, horizon_s, 0.0, 1);
    if (passes.empty()) return std::nullopt;
    const double az = azimuth_at(sat, obs, static_cast<double>(passes.front().aos));
    return RisePoint{passes.front().aos, az, compass_letters(az)};
}

std::vector<ScheduledPass> pass_schedule(const std::vector<std::pair<std::string, const SatellitePropagator*>>& sats,
                                         const Observer& obs, double from, double horizon_s, double min_el,
                                         PassFilter filter, int max_passes,
                                         const std::function<bool(const std::string&)>& include) {
    std::vector<ScheduledPass> out;
    for (const auto& [name, sat] : sats) {
        if (!sat || !sat->valid()) continue;
        if (include && !include(name)) continue;
        for (const SatPass& p : sat->passes(obs, from, horizon_s, min_el, 64)) {
            FinderPass f = describe_pass(*sat, obs, p);
            if (filter == PassFilter::VisibleToEye && !f.visible_to_eye) continue;
            if (const auto sm = standard_magnitude(name)) {
                for (double t = static_cast<double>(p.aos); t <= static_cast<double>(p.los); t += 15.0) {
                    const auto m = estimate_magnitude(*sat, obs, t, *sm);
                    if (m && (!f.brightest_mag || *m < *f.brightest_mag)) f.brightest_mag = m;
                }
            }
            out.push_back({name, f});
        }
    }
    std::sort(out.begin(), out.end(), [](const ScheduledPass& a, const ScheduledPass& b) {
        return a.pass.pass.aos < b.pass.pass.aos;
    });
    if (max_passes > 0 && static_cast<int>(out.size()) > max_passes) out.resize(static_cast<std::size_t>(max_passes));
    return out;
}

} // namespace hamclock
