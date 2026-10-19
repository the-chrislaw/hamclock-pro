#include "astro_vsop.hpp"
#include <cmath>

namespace hamclock {
namespace {
struct VsopTerm { double a, b, c; };
#include "astro_vsop/vsop87d_earth.inc"

template <std::size_t N> double sum(const VsopTerm (&s)[N], double t) {
    double v = 0;
    for (const VsopTerm& x : s) v += x.a * std::cos(x.b + x.c * t);
    return v;
}
}

EarthHelio vsop87d_earth(double jde) {
    const double t = (jde - 2451545.0) / 365250.0;  // Julian millennia
    const double t2 = t * t, t3 = t2 * t, t4 = t3 * t, t5 = t4 * t;
    double l = sum(kL0, t) + sum(kL1, t) * t + sum(kL2, t) * t2 + sum(kL3, t) * t3 + sum(kL4, t) * t4 + sum(kL5, t) * t5;
    const double b = sum(kB0, t) + sum(kB1, t) * t + sum(kB2, t) * t2 + sum(kB3, t) * t3 + sum(kB4, t) * t4 + sum(kB5, t) * t5;
    const double r = sum(kR0, t) + sum(kR1, t) * t + sum(kR2, t) * t2 + sum(kR3, t) * t3 + sum(kR4, t) * t4 + sum(kR5, t) * t5;
    const double twopi = 2 * 3.14159265358979323846;
    l = std::fmod(l, twopi);
    if (l < 0) l += twopi;
    return {l, b, r};
}
}
