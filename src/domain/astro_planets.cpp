#include "astro_planets.hpp"
#include <cmath>
#include <stdexcept>

namespace hamclock {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeltaT = 69.0; // s, UTC to TT
double rad(double d) { return d * kPi / 180.0; }
double deg(double r) { return r * 180.0 / kPi; }
double wrap360(double d) { d = std::fmod(d, 360.0); return d < 0 ? d + 360.0 : d; }

// a (AU), e, I, L, long.peri, long.node (deg) at J2000 and per Julian century.
struct Elements { double a, e, i, l, w, n, da, de, di, dl, dw, dn; };
constexpr Elements kMercury{0.38709927, 0.20563593, 7.00497902, 252.25032350, 77.45779628, 48.33076593,
                            0.00000037, 0.00001906, -0.00594749, 149472.67411175, 0.16047689, -0.12534081};
constexpr Elements kVenus{0.72333566, 0.00677672, 3.39467605, 181.97909950, 131.60246718, 76.67984255,
                          0.00000390, -0.00004107, -0.00078890, 58517.81538729, 0.00268329, -0.27769418};
constexpr Elements kEarth{1.00000261, 0.01671123, -0.00001531, 100.46457166, 102.93768193, 0.0,
                          0.00000562, -0.00004392, -0.01294668, 35999.37244981, 0.32327364, 0.0};
constexpr Elements kMars{1.52371034, 0.09339410, 1.84969142, -4.55343205, -23.94362959, 49.55953891,
                         0.00001847, 0.00007882, -0.00813131, 19140.30268499, 0.44441088, -0.29257343};
constexpr Elements kJupiter{5.20288700, 0.04838624, 1.30439695, 34.39644051, 14.72847983, 100.47390909,
                            -0.00011607, -0.00013253, -0.00183714, 3034.74612775, 0.21252668, 0.20469106};
constexpr Elements kSaturn{9.53667594, 0.05386179, 2.48599187, 49.95424423, 92.59887831, 113.66242448,
                           -0.00125060, -0.00050991, 0.00193609, 1222.49362201, -0.41897216, -0.28867794};

const Elements& elements(Planet p) {
    switch (p) {
        case Planet::Mercury: return kMercury;
        case Planet::Venus: return kVenus;
        case Planet::Mars: return kMars;
        case Planet::Jupiter: return kJupiter;
        case Planet::Saturn: return kSaturn;
    }
    return kMercury;
}

struct Vec { double x, y, z; };

// Heliocentric ecliptic J2000 rectangular coordinates, AU.
Vec heliocentric(const Elements& k, double t) {
    const double a = k.a + k.da * t, e = k.e + k.de * t, inc = rad(k.i + k.di * t);
    const double l = k.l + k.dl * t, w = k.w + k.dw * t, node = rad(k.n + k.dn * t);
    const double argp = rad(w) - node;
    double m = std::fmod(l - w, 360.0);
    if (m > 180.0) m -= 360.0; else if (m < -180.0) m += 360.0;
    m = rad(m);
    double ea = m + e * std::sin(m);
    for (int i = 0; i < 12; ++i) ea -= (ea - e * std::sin(ea) - m) / (1.0 - e * std::cos(ea));
    const double xp = a * (std::cos(ea) - e), yp = a * std::sqrt(1.0 - e * e) * std::sin(ea);
    const double ca = std::cos(argp), sa = std::sin(argp), cn = std::cos(node), sn = std::sin(node);
    const double ci = std::cos(inc), si = std::sin(inc);
    return {(ca * cn - sa * sn * ci) * xp + (-sa * cn - ca * sn * ci) * yp,
            (ca * sn + sa * cn * ci) * xp + (-sa * sn + ca * cn * ci) * yp,
            (sa * si) * xp + (ca * si) * yp};
}

double gmst_deg(double jd) {
    const double t = (jd - 2451545.0) / 36525.0;
    return wrap360(280.46061837 + 360.98564736629 * (jd - 2451545.0) + 0.000387933 * t * t);
}
void check_station(double lat, double lon) {
    if (!(lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180)) throw std::invalid_argument("station out of range");
}
double centuries(UtcSeconds utc) {
    return ((static_cast<double>(utc.value) + kDeltaT) / 86400.0 + 2440587.5 - 2451545.0) / 36525.0;
}
} // namespace

const char* planet_name(Planet p) {
    switch (p) {
        case Planet::Mercury: return "Mercury";
        case Planet::Venus: return "Venus";
        case Planet::Mars: return "Mars";
        case Planet::Jupiter: return "Jupiter";
        case Planet::Saturn: return "Saturn";
    }
    return "";
}

PlanetPosition planet_position(Planet p, UtcSeconds utc) {
    const double t = centuries(utc);
    const Vec h = heliocentric(elements(p), t), e = heliocentric(kEarth, t);
    const Vec g{h.x - e.x, h.y - e.y, h.z - e.z};
    const double dist = std::sqrt(g.x * g.x + g.y * g.y + g.z * g.z);
    const double precession = 1.3969713 * t; // general precession in longitude, deg
    const double lon = wrap360(deg(std::atan2(g.y, g.x)) + precession);
    const double lat = std::asin(g.z / dist);
    const double eps = rad(23.439291 - 0.0130042 * t);
    const double lr = rad(lon);
    const double ra = std::atan2(std::sin(lr) * std::cos(eps) - std::tan(lat) * std::sin(eps), std::cos(lr));
    const double dec = std::asin(std::sin(lat) * std::cos(eps) + std::cos(lat) * std::sin(eps) * std::sin(lr));
    const double sun_lon = wrap360(deg(std::atan2(-e.y, -e.x)) + precession);
    const double sl = rad(sun_lon);
    const double cosd = std::cos(lat) * std::cos(lr - sl);
    return {lon, wrap360(deg(ra)), deg(dec), dist, deg(std::acos(std::fmax(-1.0, std::fmin(1.0, cosd)))), sun_lon};
}

PlanetHorizontal planet_horizontal(Planet p, UtcSeconds utc, double lat, double lon) {
    check_station(lat, lon);
    const PlanetPosition q = planet_position(p, utc);
    const double jd = static_cast<double>(utc.value) / 86400.0 + 2440587.5;
    const double h = rad(gmst_deg(jd) + lon - q.ra_deg);
    const double latr = rad(lat), dec = rad(q.dec_deg);
    const double el = std::asin(std::sin(latr) * std::sin(dec) + std::cos(latr) * std::cos(dec) * std::cos(h));
    const double az = std::atan2(std::sin(h), std::cos(h) * std::sin(latr) - std::tan(dec) * std::cos(latr)) + kPi;
    return {wrap360(deg(az)), deg(el)};
}

PlanetSubpoint planet_subpoint(Planet p, UtcSeconds utc) {
    const PlanetPosition q = planet_position(p, utc);
    const double jd = static_cast<double>(utc.value) / 86400.0 + 2440587.5;
    double lon = std::fmod(q.ra_deg - gmst_deg(jd) + 540.0, 360.0) - 180.0;
    return {q.dec_deg, lon};
}
} // namespace hamclock
