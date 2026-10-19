#include "astro_eclipse.hpp"
#include "astro_elp.hpp"
#include "astro_vsop.hpp"
#include <array>
#include <cmath>
#include <stdexcept>

namespace hamclock {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeltaT = 69.0;          // TT - UTC, seconds
constexpr double kEarthRadiusKm = 6378.137;
constexpr double kFlattening = 1 / 298.257223563;
constexpr double kAuKm = 149597870.7;
constexpr double kMoonRadiusKm = 1737.4;  // penumbral (outer) contacts
constexpr double kMoonUmbralKm = 1736.5;  // k = 0.272281 Re, the NASA umbral limb for C2/C3
constexpr double kSunRadiusKm = 695700.0;
double rad(double d) { return d * kPi / 180.0; }
using Vec = std::array<double, 3>;
double dot(const Vec& a, const Vec& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
double norm(const Vec& a) { return std::sqrt(dot(a, a)); }

struct Geometry { double sep, rs, rm_pen, rm_umb, sun_elev; double moon_dist_km; };

// Equatorial vector (km) from ecliptic longitude/latitude (radians) and distance.
Vec equatorial(double lon, double lat, double dist, double eps) {
    const double x = dist * std::cos(lat) * std::cos(lon), y = dist * std::cos(lat) * std::sin(lon), z = dist * std::sin(lat);
    return {x, y * std::cos(eps) - z * std::sin(eps), y * std::sin(eps) + z * std::cos(eps)};
}

// Topocentric Sun and Moon at a UTC unix time for an observer; apparent places
// (nutation, Sun aberration). Sun is truncated VSOP87D.
Geometry geometry_at(double unix_utc, double lat_deg, double lon_deg) {
    const double jd_utc = unix_utc / 86400.0 + 2440587.5;
    const double jde = jd_utc + kDeltaT / 86400.0;
    const double t = (jde - 2451545.0) / 36525.0;
    const double om = rad(125.04452 - 1934.136261 * t);
    const double ls = rad(280.4665 + 36000.7698 * t), lm = rad(218.3165 + 481267.8813 * t);
    const double dpsi = rad((-17.20 * std::sin(om) - 1.32 * std::sin(2 * ls) - 0.23 * std::sin(2 * lm) + 0.21 * std::sin(2 * om)) / 3600.0);
    const double deps = rad((9.20 * std::cos(om) + 0.57 * std::cos(2 * ls) + 0.10 * std::cos(2 * lm) - 0.09 * std::cos(2 * om)) / 3600.0);
    const double eps0 = rad(23.439291 - 0.0130042 * t - 1.64e-7 * t * t);
    const double eps = eps0 + deps;
    // Sun: VSOP87D Earth reversed, FK5 correction (Meeus ch. 25), nutation, aberration
    const EarthHelio ea = vsop87d_earth(jde);
    const double as_ = kPi / 180.0 / 3600.0;
    const double lp = ea.longitude_rad - rad(1.397) * t - rad(0.00031) * t * t;
    const double hel_lon = ea.longitude_rad - 0.09033 * as_;
    const double hel_lat = ea.latitude_rad + 0.03916 * as_ * (std::cos(lp) - std::sin(lp));
    const double sun_lon = hel_lon + kPi + dpsi - 20.4898 * as_ / ea.radius_au;
    const double sun_lat = -hel_lat;
    const Vec sun = equatorial(sun_lon, sun_lat, ea.radius_au * kAuKm, eps);
    // Moon
    // J2000 ecliptic to the mean ecliptic of date (Meeus ch. 21: rotation by eta about the node Pi, plus p)
    const ElpPosition mp = elp_moon(jde);
    const double as = kPi / 180.0 / 3600.0;
    const double eta = as * (47.0029 * t - 0.03302 * t * t), pi_ = rad(174.876384) + as * (-869.8089 * t + 0.03536 * t * t);
    const double pa = as * (5029.0966 * t + 1.11113 * t * t);
    const double sa = std::cos(eta) * std::cos(mp.latitude_rad) * std::sin(pi_ - mp.longitude_rad) - std::sin(eta) * std::sin(mp.latitude_rad);
    const double sb = std::cos(mp.latitude_rad) * std::cos(pi_ - mp.longitude_rad);
    const double sc = std::cos(eta) * std::sin(mp.latitude_rad) + std::sin(eta) * std::cos(mp.latitude_rad) * std::sin(pi_ - mp.longitude_rad);
    const Vec moon = equatorial(pa + pi_ - std::atan2(sa, sb) + dpsi, std::asin(sc), mp.distance_km, eps);
    // Apparent sidereal time (Meeus 12.4 plus equation of the equinoxes)
    const double d = jd_utc - 2451545.0, tu = d / 36525.0;
    double gmst = 280.46061837 + 360.98564736629 * d + 0.000387933 * tu * tu;
    gmst = rad(std::fmod(gmst, 360.0)) + dpsi * std::cos(eps);
    const double theta = gmst + rad(lon_deg);
    // Observer geocentric position (km)
    const double la = rad(lat_deg), f1 = 1 - kFlattening;
    const double u = std::atan(f1 * std::tan(la));
    const double rho_sin = f1 * std::sin(u), rho_cos = std::cos(u);
    const Vec obs{kEarthRadiusKm * rho_cos * std::cos(theta), kEarthRadiusKm * rho_cos * std::sin(theta), kEarthRadiusKm * rho_sin};
    const Vec ts{sun[0] - obs[0], sun[1] - obs[1], sun[2] - obs[2]}, tm{moon[0] - obs[0], moon[1] - obs[1], moon[2] - obs[2]};
    Geometry g{};
    const double ds = norm(ts), dm = norm(tm);
    g.sep = std::acos(std::fmax(-1.0, std::fmin(1.0, dot(ts, tm) / (ds * dm))));
    g.rs = std::asin(kSunRadiusKm / ds);
    g.rm_pen = std::asin(kMoonRadiusKm / dm);
    g.rm_umb = std::asin(kMoonUmbralKm / dm);
    g.moon_dist_km = dm;
    const Vec zen{std::cos(la) * std::cos(theta), std::cos(la) * std::sin(theta), std::sin(la)}; // geocentric zenith, ample for 'above horizon'
    g.sun_elev = std::asin(dot(ts, zen) / ds);
    return g;
}
} // namespace

EclipseContacts eclipse_contacts(const SolarEclipse& ecl, double lat, double lon) {
    if (!std::isfinite(lat) || !std::isfinite(lon) || lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
        throw std::invalid_argument("invalid location");
    EclipseContacts out{};
    const double t0 = static_cast<double>(ecl.greatest.value);
    const auto sep = [&](double t) { return geometry_at(t, lat, lon).sep; };
    // Minimum separation: coarse scan then golden-section refinement.
    double best_t = t0, best = 1e9;
    for (int s = -240; s <= 240; ++s) {
        const double t = t0 + s * 60.0, v = sep(t);
        if (v < best) { best = v; best_t = t; }
    }
    double a = best_t - 60, b = best_t + 60;
    const double gr = (std::sqrt(5.0) - 1) / 2;
    for (int i = 0; i < 40; ++i) {
        const double c = b - gr * (b - a), d = a + gr * (b - a);
        if (sep(c) < sep(d)) b = d; else a = c;
    }
    const double tmax = (a + b) / 2;
    const Geometry gm = geometry_at(tmax, lat, lon);
    out.maximum = UtcSeconds{static_cast<std::int64_t>(std::llround(tmax))};
    out.min_separation_deg = gm.sep * 180.0 / kPi;
    out.sun_up_at_maximum = gm.sun_elev > 0;
    const double outer = gm.rs + gm.rm_pen;
    out.magnitude = std::fmax(0.0, (outer - gm.sep) / (2 * gm.rs));
    if (gm.sep >= outer) return out;
    // Root of f(t) = sep - limit(t) between a point inside (tmax) and one outside.
    const auto root = [&](double inside, double outside, bool umbral) {
        const auto f = [&](double t) { const Geometry g = geometry_at(t, lat, lon);
            return g.sep - (umbral ? std::fabs(g.rm_umb - g.rs) : g.rs + g.rm_pen); };
        for (int i = 0; i < 50; ++i) {
            const double mid = (inside + outside) / 2;
            if (f(mid) < 0) inside = mid; else outside = mid;
        }
        return UtcSeconds{static_cast<std::int64_t>(std::llround((inside + outside) / 2))};
    };
    out.partial = true;
    out.first = root(tmax, tmax - 4 * 3600.0, false);
    out.last = root(tmax, tmax + 4 * 3600.0, false);
    const double inner = std::fabs(gm.rm_umb - gm.rs);
    if (gm.sep < inner) {
        out.central = true;
        out.total = gm.rm_umb > gm.rs;
        out.second = root(tmax, tmax - 600.0, true);
        out.third = root(tmax, tmax + 600.0, true);
    }
    return out;
}
}
