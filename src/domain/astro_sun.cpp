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

namespace hamclock {
std::vector<SunYearDay> sun_year_series(int year, double latitude_deg, double longitude_deg) {
    if (year < 1900 || year > 2100) throw std::invalid_argument("year out of range");
    // Days from 1970-01-01 to Jan 1 of `year` (civil-from-days inverse, Hinnant).
    const int y = year - 1;
    const std::int64_t jan1 = std::int64_t(y / 400) * 146097 + (y % 400) * 365 + (y % 400) / 4 - (y % 400) / 100 - 719162 + 0;
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    std::vector<SunYearDay> out;
    out.reserve(leap ? 366 : 365);
    for (int d = 0; d < (leap ? 366 : 365); ++d) {
        const std::int64_t start = (jan1 + d) * 86400;
        const SunDay day = sun_events_for_utc_day(start, latitude_deg, longitude_deg);
        SunYearDay e{d + 1, -1, -1, day.polar};
        for (const auto& ev : day.events) {
            const double m = double(ev.time.value - start) / 60.0;
            (ev.kind == SunEventKind::Rise ? e.rise_min : e.set_min) = m;
        }
        out.push_back(e);
    }
    return out;
}
}

namespace hamclock {
SeasonEvent season_event(int year, SeasonKind kind) {
    static const double mean[4][5] = {
        {2451623.80984, 365242.37404, 0.05169, -0.00411, -0.00057},
        {2451716.56767, 365241.62603, 0.00325, 0.00888, -0.00030},
        {2451810.21715, 365242.01767, -0.11575, 0.00337, 0.00078},
        {2451900.05952, 365242.74049, -0.06223, -0.00823, 0.00032}};
    static const double terms[24][3] = {
        {485, 324.96, 1934.136}, {203, 337.23, 32964.467}, {199, 342.08, 20.186},
        {182, 27.85, 445267.112}, {156, 73.14, 45036.886}, {136, 171.52, 22518.443},
        {77, 222.54, 65928.934}, {74, 296.72, 3034.906}, {70, 243.58, 9037.513},
        {58, 119.81, 33718.147}, {52, 297.17, 150.678}, {50, 21.02, 2281.226},
        {45, 247.54, 29929.562}, {44, 325.15, 31555.956}, {29, 60.93, 4443.417},
        {18, 155.12, 67555.328}, {17, 288.79, 4562.452}, {16, 198.04, 62894.029},
        {14, 199.76, 31436.921}, {12, 95.39, 14577.848}, {12, 287.11, 31931.756},
        {12, 320.81, 34777.259}, {9, 227.73, 1222.114}, {8, 15.45, 16859.074}};
    const double rad = 3.14159265358979323846 / 180.0;
    const double* m = mean[static_cast<int>(kind)];
    const double y = (year - 2000) / 1000.0;
    const double jde0 = m[0] + y * (m[1] + y * (m[2] + y * (m[3] + y * m[4])));
    const double t = (jde0 - 2451545.0) / 36525.0;
    const double w = (35999.373 * t - 2.47) * rad;
    const double dl = 1 + 0.0334 * std::cos(w) + 0.0007 * std::cos(2 * w);
    double s = 0;
    for (const auto& k : terms) s += k[0] * std::cos((k[1] + k[2] * t) * rad);
    const double jde = jde0 + 0.00001 * s / dl;
    constexpr double kDeltaT = 69.0; // TT - UTC, seconds (as in astro_moon.cpp)
    const double unix_tt = (jde - 2440587.5) * 86400.0;
    return {kind, UtcSeconds{static_cast<std::int64_t>(std::llround(unix_tt - kDeltaT))}};
}

SeasonEvent next_season_event(UtcSeconds after) {
    const int year = 1970 + static_cast<int>(after.value / 31556952);
    for (int y = year - 1; y <= year + 1; ++y)
        for (int k = 0; k < 4; ++k) {
            const SeasonEvent e = season_event(y, static_cast<SeasonKind>(k));
            if (e.time.value > after.value) return e;
        }
    return season_event(year + 2, SeasonKind::MarchEquinox);
}
}
