#include "astro_elp.hpp"
#include <cmath>
#include <cstdint>

namespace hamclock {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg = kPi / 180.0;
constexpr double kArcsec = kPi / 648000.0;

struct ElpMain { std::int8_t ilu[4]; double amp; };
// pow: the amplitude is multiplied by t^pow; arg: Delaunay D, l', l, F then
// the zeta (precession-bearing) multiplier then the eight planetary longitudes.
struct ElpTerm { std::int8_t pow; double phase_deg; double amp; std::int8_t arg[13]; };

#include "astro_elp/elp82b_terms.inc"

template <typename T, std::size_t N> constexpr std::size_t count(const T (&)[N]) { return N; }

// Fundamental arguments as polynomials in Julian centuries since J2000.
struct Args {
    double del[4][5];  // Delaunay D, l', l, F (up to t^4)
    double zeta[2];
    double p[8][2];
    double w1[5];      // mean longitude of the Moon
};

Args make_args() {
    const double w[3][5] = {
        {(218 + 18 / 60.0 + 59.95571 / 3600) * kDeg, 1732559343.73604 * kArcsec, -5.8883 * kArcsec, 0.6604e-2 * kArcsec, -0.3169e-4 * kArcsec},
        {(83 + 21 / 60.0 + 11.67475 / 3600) * kDeg, 14643420.2632 * kArcsec, -38.2776 * kArcsec, -0.45047e-1 * kArcsec, 0.21301e-3 * kArcsec},
        {(125 + 2 / 60.0 + 40.39816 / 3600) * kDeg, -6967919.3622 * kArcsec, 6.3622 * kArcsec, 0.7625e-2 * kArcsec, -0.3586e-4 * kArcsec}};
    const double eart[5] = {(100 + 27 / 60.0 + 59.22059 / 3600) * kDeg, 129597742.2758 * kArcsec, -0.0202 * kArcsec, 0.9e-5 * kArcsec, 0.15e-6 * kArcsec};
    const double peri[5] = {(102 + 56 / 60.0 + 14.42753 / 3600) * kDeg, 1161.2283 * kArcsec, 0.5327 * kArcsec, -0.138e-3 * kArcsec, 0.0};
    Args a{};
    for (int i = 0; i < 5; ++i) {
        a.del[0][i] = w[0][i] - eart[i];
        a.del[1][i] = eart[i] - peri[i];
        a.del[2][i] = w[0][i] - w[1][i];
        a.del[3][i] = w[0][i] - w[2][i];
        a.w1[i] = w[0][i];
    }
    a.del[0][0] += kPi;
    a.zeta[0] = w[0][0];
    a.zeta[1] = w[0][1] + 5029.0966 * kArcsec;
    const double p0[8] = {(252 + 15 / 60.0 + 3.25986 / 3600) * kDeg, (181 + 58 / 60.0 + 47.28305 / 3600) * kDeg, eart[0],
                          (355 + 25 / 60.0 + 59.78866 / 3600) * kDeg, (34 + 21 / 60.0 + 5.34212 / 3600) * kDeg,
                          (50 + 4 / 60.0 + 38.89694 / 3600) * kDeg, (314 + 3 / 60.0 + 18.01841 / 3600) * kDeg,
                          (304 + 20 / 60.0 + 55.19575 / 3600) * kDeg};
    const double p1[8] = {538101628.68898 * kArcsec, 210664136.43355 * kArcsec, eart[1], 68905077.59284 * kArcsec,
                          10925660.42861 * kArcsec, 4399609.65932 * kArcsec, 1542481.19393 * kArcsec, 786550.32074 * kArcsec};
    for (int i = 0; i < 8; ++i) { a.p[i][0] = p0[i]; a.p[i][1] = p1[i]; }
    return a;
}

double sum_main(const ElpMain* terms, std::size_t n, const Args& a, const double (&t)[5], double phase) {
    // Collapse each Delaunay argument to its value at t once.
    double d[4];
    for (int i = 0; i < 4; ++i) {
        d[i] = 0;
        for (int k = 0; k < 5; ++k) d[i] += a.del[i][k] * t[k];
    }
    double s = 0;
    for (std::size_t j = 0; j < n; ++j) {
        double y = phase;
        for (int i = 0; i < 4; ++i) y += terms[j].ilu[i] * d[i];
        s += terms[j].amp * std::sin(std::fmod(y, 2 * kPi));
    }
    return s;
}

double sum_gen(const ElpTerm* terms, std::size_t n, const Args& a, double t) {
    double f[13];
    for (int i = 0; i < 4; ++i) f[i] = a.del[i][0] + a.del[i][1] * t;
    f[4] = a.zeta[0] + a.zeta[1] * t;
    for (int i = 0; i < 8; ++i) f[5 + i] = a.p[i][0] + a.p[i][1] * t;
    double s = 0;
    for (std::size_t j = 0; j < n; ++j) {
        double y = terms[j].phase_deg * kDeg;
        for (int i = 0; i < 13; ++i) y += terms[j].arg[i] * f[i];
        double amp = terms[j].amp;
        for (int q = 0; q < terms[j].pow; ++q) amp *= t;
        s += amp * std::sin(std::fmod(y, 2 * kPi));
    }
    return s;
}
}  // namespace

ElpPosition elp_moon(double jde) {
    static const Args a = make_args();
    const double tt = (jde - 2451545.0) / 36525.0;
    const double t[5] = {1.0, tt, tt * tt, tt * tt * tt, tt * tt * tt * tt};
    const double lon = sum_main(kMainLon, count(kMainLon), a, t, 0.0) + sum_gen(kGenLon, count(kGenLon), a, tt);
    const double lat = sum_main(kMainLat, count(kMainLat), a, t, 0.0) + sum_gen(kGenLat, count(kGenLat), a, tt);
    const double dist = sum_main(kMainDist, count(kMainDist), a, t, kPi / 2) + sum_gen(kGenDist, count(kGenDist), a, tt);
    double w1 = 0;
    for (int k = 0; k < 5; ++k) w1 += a.w1[k] * t[k];
    const double a0 = 384747.9806448954, ath = 384747.9806743165;
    return {std::fmod(lon * kArcsec + w1, 2 * kPi) + (std::fmod(lon * kArcsec + w1, 2 * kPi) < 0 ? 2 * kPi : 0.0), lat * kArcsec, dist * a0 / ath};
}

ElpPosition elp_moon_mean_of_date(double jde) {
    ElpPosition p = elp_moon(jde);
    const double tt = (jde - 2451545.0) / 36525.0;
    p.longitude_rad = std::fmod(p.longitude_rad + (5029.0966 + 1.1120 * tt) * tt * kArcsec + 2 * kPi, 2 * kPi);
    return p;
}
}
