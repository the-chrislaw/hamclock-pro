#include "astro_eme.hpp"
#include <cmath>
namespace hamclock {
double eme_degradation_db(double distance_km) {
    if (distance_km <= kMoonPerigeeKm) return 0.0;
    return 40.0 * std::log10(distance_km / kMoonPerigeeKm);
}

double moon_sun_separation_deg(UtcSeconds utc) {
    // Sun's ecliptic latitude is ~0, so cos(sep) = cos(beta_moon) cos(elongation).
    const MoonPosition m = moon_position(utc);
    const MoonPhase ph = moon_phase(utc);
    const double r = 3.14159265358979323846 / 180.0;
    double c = std::cos(m.latitude_deg * r) * std::cos(ph.elongation_deg * r);
    c = std::fmax(-1.0, std::fmin(1.0, c));
    return std::acos(c) / r;
}

std::optional<LunarApsis> next_lunar_apsis(UtcSeconds after, bool perigee) {
    const double sign = perigee ? 1.0 : -1.0;  // minimise sign * distance
    auto f = [&](std::int64_t t) { return sign * moon_position({t}).distance_km; };
    constexpr std::int64_t step = 3 * 3600;
    std::int64_t a = after.value, b = a + step, c = b + step;
    double fa = f(a), fb = f(b), fc = f(c);
    for (; a < after.value + 32LL * 86400; a = b, b = c, c += step, fa = fb, fb = fc, fc = f(c)) {
        if (!(fb < fa && fb <= fc)) continue;
        std::int64_t lo = a, hi = c;
        while (hi - lo > 30) {  // ternary search on a unimodal bracket
            const std::int64_t m1 = lo + (hi - lo) / 3, m2 = hi - (hi - lo) / 3;
            if (f(m1) < f(m2)) hi = m2; else lo = m1;
        }
        const std::int64_t t = (lo + hi) / 2;
        if (t <= after.value) continue;
        return LunarApsis{{t}, moon_position({t}).distance_km};
    }
    return std::nullopt;
}

namespace {
bool both_up(std::int64_t t, double la1, double lo1, double la2, double lo2) {
    return moon_horizontal({t}, la1, lo1).elevation_deg > 0.0 && moon_horizontal({t}, la2, lo2).elevation_deg > 0.0;
}
// Bisect the transition between t_lo (state `lo_state`) and t_hi (the other state).
std::int64_t edge(std::int64_t t_lo, std::int64_t t_hi, bool lo_state, double la1, double lo1, double la2, double lo2) {
    while (t_hi - t_lo > 1) {
        const std::int64_t mid = t_lo + (t_hi - t_lo) / 2;
        if (both_up(mid, la1, lo1, la2, lo2) == lo_state) t_lo = mid; else t_hi = mid;
    }
    return t_hi;
}
} // namespace

std::optional<MutualWindow> next_mutual_moon_window(UtcSeconds after, double la1, double lo1, double la2, double lo2, int horizon_hours) {
    constexpr std::int64_t kStep = 300; // shorter than any plausible mutual window
    const std::int64_t t0 = after.value, tend = t0 + std::int64_t(horizon_hours) * 3600;
    const bool up0 = both_up(t0, la1, lo1, la2, lo2);
    std::int64_t start = t0;
    bool in_progress = up0;
    if (!up0) {
        std::int64_t prev = t0, t = t0 + kStep;
        for (;; prev = t, t += kStep) {
            if (prev >= tend) return std::nullopt;
            if (both_up(t, la1, lo1, la2, lo2)) { start = edge(prev, t, false, la1, lo1, la2, lo2); break; }
        }
    }
    std::int64_t prev = start, t = start + kStep;
    for (;; prev = t, t += kStep) {
        if (!both_up(t, la1, lo1, la2, lo2)) return MutualWindow{{start}, {edge(prev, t, true, la1, lo1, la2, lo2)}, in_progress};
        if (t > tend + 86400) return std::nullopt; // window runs past any sane length
    }
}
}
