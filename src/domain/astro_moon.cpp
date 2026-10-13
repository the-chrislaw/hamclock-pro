#include "astro_moon.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace hamclock {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeltaT = 69.0;
constexpr double kEarthRadiusKm = 6378.14;
double rad(double d) { return d * kPi / 180.0; }
double deg(double r) { return r * 180.0 / kPi; }
double wrap360(double d) { d = std::fmod(d, 360.0); return d < 0 ? d + 360.0 : d; }
double jd_utc(double unix_seconds) { return unix_seconds / 86400.0 + 2440587.5; }

struct Term { int d, m, mp, f; double coeff; };
// Longitude (1e-6 deg) and distance (1e-3 km) share arguments; latitude differs.
struct LonDist { int d, m, mp, f; double lon, dist; };
constexpr LonDist kLonDist[] = {
    {0, 0, 1, 0, 6288774, -20905355}, {2, 0, -1, 0, 1274027, -3699111}, {2, 0, 0, 0, 658314, -2955968},
    {0, 0, 2, 0, 213618, -569925},    {0, 1, 0, 0, -185116, 48888},     {0, 0, 0, 2, -114332, -3149},
    {2, 0, -2, 0, 58793, 246158},     {2, -1, -1, 0, 57066, -152138},   {2, 0, 1, 0, 53322, -170733},
    {2, -1, 0, 0, 45758, -204586},    {0, 1, -1, 0, -40923, -129620},   {1, 0, 0, 0, -34720, 108743},
    {0, 1, 1, 0, -30383, 104755},     {2, 0, 0, -2, 15327, 10321},      {0, 0, 1, 2, -12528, 0},
    {0, 0, 1, -2, 10980, 79661},      {4, 0, -1, 0, 10675, -34782},     {0, 0, 3, 0, 10034, -23210},
    {4, 0, -2, 0, 8548, -21636},      {2, 1, -1, 0, -7888, 24208},      {2, 1, 0, 0, -6766, 30824},
    {1, 0, -1, 0, -5163, -8379},      {1, 1, 0, 0, 4987, -16675},       {2, -1, 1, 0, 4036, -12831},
    {2, 0, 2, 0, 3994, -10445},       {4, 0, 0, 0, 3861, -11650},       {2, 0, -3, 0, 3665, 14403},
    {0, 1, -2, 0, -2689, -7003},
};
constexpr Term kLat[] = {
    {0, 0, 0, 1, 5128122}, {0, 0, 1, 1, 280602}, {0, 0, 1, -1, 277693}, {2, 0, 0, -1, 173237},
    {2, 0, -1, 1, 55413},  {2, 0, -1, -1, 46271}, {2, 0, 0, 1, 32573},   {0, 0, 2, 1, 17198},
    {2, 0, 1, -1, 9266},   {0, 0, 2, -1, 8822},   {2, -1, 0, -1, 8216},  {2, 0, -2, -1, 4324},
    {2, 0, 1, 1, 4200},
};

double gmst_deg(double jd) {
    const double t = (jd - 2451545.0) / 36525.0;
    return wrap360(280.46061837 + 360.98564736629 * (jd - 2451545.0) + 0.000387933 * t * t);
}
double mean_obliquity_deg(double t) { return 23.439291 - 0.0130042 * t; }

MoonPosition position_at_jde(double jde) {
    const double t = (jde - 2451545.0) / 36525.0;
    const double lp = wrap360(218.3164477 + 481267.88123421 * t);
    const double d = rad(wrap360(297.8501921 + 445267.1114034 * t));
    const double m = rad(wrap360(357.5291092 + 35999.0502909 * t));
    const double mp = rad(wrap360(134.9633964 + 477198.8675055 * t));
    const double f = rad(wrap360(93.2720950 + 483202.0175233 * t));
    const double a1 = rad(wrap360(119.75 + 131.849 * t));
    const double a2 = rad(wrap360(53.09 + 479264.290 * t));
    const double a3 = rad(wrap360(313.45 + 481266.484 * t));
    const double e = 1.0 - 0.002516 * t - 0.0000074 * t * t;
    auto ef = [&](int mm) { return mm == 0 ? 1.0 : (std::abs(mm) == 1 ? e : e * e); };
    double sl = 0, sr = 0, sb = 0;
    for (const auto& k : kLonDist) {
        const double arg = k.d * d + k.m * m + k.mp * mp + k.f * f;
        sl += k.lon * ef(k.m) * std::sin(arg);
        sr += k.dist * ef(k.m) * std::cos(arg);
    }
    for (const auto& k : kLat) sb += k.coeff * ef(k.m) * std::sin(k.d * d + k.m * m + k.mp * mp + k.f * f);
    const double lpr = rad(lp);
    sl += 3958 * std::sin(a1) + 1962 * std::sin(lpr - f) + 318 * std::sin(a2);
    sb += -2235 * std::sin(lpr) + 382 * std::sin(a3) + 175 * std::sin(a1 - f) + 175 * std::sin(a1 + f) +
          127 * std::sin(lpr - mp) - 115 * std::sin(lpr + mp);
    MoonPosition p;
    p.longitude_deg = wrap360(lp + sl / 1e6);
    p.latitude_deg = sb / 1e6;
    p.distance_km = 385000.56 + sr / 1000.0;
    const double eps = rad(mean_obliquity_deg(t));
    const double lam = rad(p.longitude_deg), bet = rad(p.latitude_deg);
    p.ra_deg = wrap360(deg(std::atan2(std::sin(lam) * std::cos(eps) - std::tan(bet) * std::sin(eps), std::cos(lam))));
    p.dec_deg = deg(std::asin(std::sin(bet) * std::cos(eps) + std::cos(bet) * std::sin(eps) * std::sin(lam)));
    return p;
}

// Low-precision apparent solar longitude and distance (km), Meeus ch. 25.
void sun_lon_dist(double jde, double& lon_deg, double& dist_km) {
    const double t = (jde - 2451545.0) / 36525.0;
    const double l0 = 280.46646 + 36000.76983 * t;
    const double m = rad(357.52911 + 35999.05029 * t);
    const double e = 0.016708634 - 0.000042037 * t;
    const double c = (1.914602 - 0.004817 * t) * std::sin(m) + 0.019993 * std::sin(2 * m) + 0.000289 * std::sin(3 * m);
    const double nu = m + rad(c);
    lon_deg = wrap360(l0 + c - 0.00569 - 0.00478 * std::sin(rad(125.04 - 1934.136 * t)));
    dist_km = 149597870.7 * 1.000001018 * (1 - e * e) / (1 + e * std::cos(nu));
}

void check_station(double lat, double lon) {
    if (!(lat >= -90 && lat <= 90) || !(lon >= -180 && lon <= 180)) throw std::invalid_argument("latitude/longitude out of range");
}
double jde_of(UtcSeconds t) { return jd_utc(static_cast<double>(t.value)) + kDeltaT / 86400.0; }

// Altitude of the upper limb minus the rise/set threshold: positive when risen.
double rise_margin(double unix_s, double lat, double lon) {
    const double jd = jd_utc(unix_s);
    const MoonPosition p = position_at_jde(jd + kDeltaT / 86400.0);
    const double h = rad(gmst_deg(jd) + lon - p.ra_deg);
    const double latr = rad(lat), dec = rad(p.dec_deg);
    const double geo = deg(std::asin(std::sin(latr) * std::sin(dec) + std::cos(latr) * std::cos(dec) * std::cos(h)));
    const double hp = deg(std::asin(kEarthRadiusKm / p.distance_km));
    return geo - (0.7275 * hp - 0.5667);
}
} // namespace

MoonPosition moon_position(UtcSeconds utc) { return position_at_jde(jde_of(utc)); }

MoonHorizontal moon_horizontal(UtcSeconds utc, double lat, double lon) {
    check_station(lat, lon);
    const MoonPosition p = position_at_jde(jde_of(utc));
    const double h = rad(gmst_deg(jd_utc(static_cast<double>(utc.value))) + lon - p.ra_deg);
    const double latr = rad(lat), dec = rad(p.dec_deg);
    const double geo = std::asin(std::sin(latr) * std::sin(dec) + std::cos(latr) * std::cos(dec) * std::cos(h));
    const double az = std::atan2(std::sin(h), std::cos(h) * std::sin(latr) - std::tan(dec) * std::cos(latr)) + kPi;
    const double parallax = std::asin(kEarthRadiusKm / p.distance_km) * std::cos(geo);
    return {wrap360(deg(az)), deg(geo - parallax)};
}

MoonPhase moon_phase(UtcSeconds utc) {
    const double jde = jde_of(utc);
    const MoonPosition m = position_at_jde(jde);
    double sun_lon, sun_dist;
    sun_lon_dist(jde, sun_lon, sun_dist);
    MoonPhase out;
    out.elongation_deg = wrap360(m.longitude_deg - sun_lon);
    const double psi = std::acos(std::cos(rad(m.latitude_deg)) * std::cos(rad(out.elongation_deg)));
    const double i = std::atan2(sun_dist * std::sin(psi), m.distance_km - sun_dist * std::cos(psi));
    out.illuminated = (1 + std::cos(i)) / 2;
    out.waxing = out.elongation_deg < 180.0;
    const double e = out.elongation_deg;
    if (e < 10 || e >= 350) out.name = "New Moon";
    else if (e < 80) out.name = "Waxing Crescent";
    else if (e < 100) out.name = "First Quarter";
    else if (e < 170) out.name = "Waxing Gibbous";
    else if (e < 190) out.name = "Full Moon";
    else if (e < 260) out.name = "Waning Gibbous";
    else if (e < 280) out.name = "Last Quarter";
    else out.name = "Waning Crescent";
    return out;
}

std::vector<MoonEvent> moon_events_for_utc_day(std::int64_t day_start, double lat, double lon) {
    check_station(lat, lon);
    std::vector<MoonEvent> out;
    constexpr double step = 600.0; // the Moon crosses the horizon at most about twice a day
    double t0 = static_cast<double>(day_start);
    double a = rise_margin(t0, lat, lon);
    for (double t1 = t0 + step; t1 <= t0 + 86400.0 + 1e-6; t1 += step) {
        const double b = rise_margin(t1, lat, lon);
        if ((a < 0) != (b < 0)) {
            double lo = t1 - step, hi = t1;
            const bool rising = b >= 0;
            for (int i = 0; i < 30; ++i) {
                const double mid = (lo + hi) / 2;
                ((rise_margin(mid, lat, lon) >= 0) == rising ? hi : lo) = mid;
            }
            const auto sec = static_cast<std::int64_t>(std::llround((lo + hi) / 2));
            if (sec >= day_start && sec < day_start + 86400)
                out.push_back({rising ? MoonEventKind::Rise : MoonEventKind::Set, UtcSeconds{sec}});
        }
        a = b;
    }
    return out;
}

std::optional<UtcSeconds> next_moon_event(UtcSeconds after, MoonEventKind kind, double lat, double lon) {
    std::int64_t day = after.value - ((after.value % 86400) + 86400) % 86400;
    for (int i = 0; i < 4; ++i, day += 86400)
        for (const auto& e : moon_events_for_utc_day(day, lat, lon))
            if (e.kind == kind && e.time.value > after.value) return e.time;
    return std::nullopt;
}
} // namespace hamclock
