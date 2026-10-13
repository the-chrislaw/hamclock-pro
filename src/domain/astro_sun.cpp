#include "astro_sun.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace hamclock {
namespace {
constexpr double kPi = 3.14159265358979323846;
double rad(double d) { return d * kPi / 180.0; }
double deg(double r) { return r * 180.0 / kPi; }

struct SunState { double eq_minutes; double declination_rad; };
SunState sun_state(double jd) {
    const double t = (jd - 2451545.0) / 36525.0;
    const double l0 = std::fmod(280.46646 + t * (36000.76983 + t * 0.0003032), 360.0);
    const double m = 357.52911 + t * (35999.05029 - 0.0001537 * t);
    const double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
    const double mr = rad(m);
    const double c = std::sin(mr) * (1.914602 - t * (0.004817 + 0.000014 * t)) +
                     std::sin(2 * mr) * (0.019993 - 0.000101 * t) + std::sin(3 * mr) * 0.000289;
    const double true_long = l0 + c;
    const double omega = 125.04 - 1934.136 * t;
    const double app_long = true_long - 0.00569 - 0.00478 * std::sin(rad(omega));
    const double eps0 = 23.0 + (26.0 + (21.448 - t * (46.815 + t * (0.00059 - t * 0.001813))) / 60.0) / 60.0;
    const double eps = eps0 + 0.00256 * std::cos(rad(omega));
    const double decl = std::asin(std::sin(rad(eps)) * std::sin(rad(app_long)));
    const double y = std::tan(rad(eps / 2)) * std::tan(rad(eps / 2));
    const double l0r = rad(l0);
    const double eq = 4.0 * deg(y * std::sin(2 * l0r) - 2 * e * std::sin(mr) +
                                4 * e * y * std::sin(mr) * std::cos(2 * l0r) -
                                0.5 * y * y * std::sin(4 * l0r) - 1.25 * e * e * std::sin(2 * mr));
    return {eq, decl};
}
double jd_of(double unix_seconds) { return unix_seconds / 86400.0 + 2440587.5; }

// Event time in unix seconds near `guess`, or nullopt if the sun does not cross.
// `cos_ha` out-param reports the polar condition from the last iteration.
std::optional<double> solve(double day_start, double lat, double lon, SunEventKind kind, double& cos_ha) {
    double t = day_start + (kind == SunEventKind::Rise ? 6 : 18) * 3600.0 - lon * 240.0;
    for (int i = 0; i < 3; ++i) {
        const SunState s = sun_state(jd_of(t));
        cos_ha = (std::cos(rad(90.833)) - std::sin(rad(lat)) * std::sin(s.declination_rad)) /
                 (std::cos(rad(lat)) * std::cos(s.declination_rad));
        if (cos_ha > 1.0 || cos_ha < -1.0) return std::nullopt;
        const double ha = deg(std::acos(cos_ha));
        const double minutes = 720.0 - 4.0 * (lon + (kind == SunEventKind::Rise ? ha : -ha)) - s.eq_minutes;
        t = day_start + minutes * 60.0;
    }
    return t;
}
} // namespace

SunDay sun_events_for_utc_day(std::int64_t day_start_utc, double lat, double lon) {
    if (!(lat >= -90 && lat <= 90) || !(lon >= -180 && lon <= 180)) throw std::invalid_argument("latitude/longitude out of range");
    SunDay out;
    double cos_noon = 0;
    for (std::int64_t offset : {-86400LL, 0LL, 86400LL}) {
        for (SunEventKind kind : {SunEventKind::Rise, SunEventKind::Set}) {
            double cos_ha = 0;
            const auto t = solve(static_cast<double>(day_start_utc + offset), lat, lon, kind, cos_ha);
            if (offset == 0 && kind == SunEventKind::Rise) cos_noon = cos_ha;
            if (!t) continue;
            const auto sec = static_cast<std::int64_t>(std::llround(*t));
            if (sec >= day_start_utc && sec < day_start_utc + 86400) out.events.push_back({kind, UtcSeconds{sec}});
        }
    }
    std::sort(out.events.begin(), out.events.end(), [](const SunEvent& a, const SunEvent& b) { return a.time.value < b.time.value; });
    if (out.events.empty()) out.polar = cos_noon < -1.0 ? PolarState::MidnightSun : PolarState::PolarNight;
    return out;
}

std::optional<UtcSeconds> next_sun_event(UtcSeconds after, SunEventKind kind, double lat, double lon) {
    std::int64_t day = after.value - ((after.value % 86400) + 86400) % 86400;
    for (int i = 0; i < 3; ++i, day += 86400)
        for (const auto& e : sun_events_for_utc_day(day, lat, lon).events)
            if (e.kind == kind && e.time.value > after.value) return e.time;
    return std::nullopt;
}
} // namespace hamclock
