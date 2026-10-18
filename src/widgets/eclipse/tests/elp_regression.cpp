// Truncated ELP2000-82B: checked against the full theory (computed offline
// from the CDS VI/79 tables with a direct port of elp82b.f) and against
// Meeus, Astronomical Algorithms ch. 47, example 47.a.
#include "../../../domain/astro_elp.hpp"
#include "../../../domain/astro_moon.hpp"
#include <cmath>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
double wrap_diff(double a, double b) {
    double d = std::fmod(a - b, 2 * M_PI);
    if (d > M_PI) d -= 2 * M_PI;
    if (d < -M_PI) d += 2 * M_PI;
    return d;
}
struct Ref { double jde, lon, lat, dist; };
// Full-theory values: longitude (J2000 inertial, rad mod 2 pi), latitude, km.
const Ref kRefs[] = {
    {2448724.5, 2.326003933, -0.056360217, 368405.5612},
    {2460409.26, 0.331331908, 0.005969364, 359797.3621},
    {2461000.0, 4.195824640, -0.084527828, 406644.0129},
    {2451545.0, 3.897651135, 0.090248664, 402448.6658},
    {2470000.0, 0.586601211, 0.022651048, 377192.1757},
};
}

int main() {
    using namespace hamclock;
    for (const Ref& r : kRefs) {
        const ElpPosition p = elp_moon(r.jde);
        // 1e-6 rad is 0.2 arcsec, about 0.1 s of lunar motion.
        check(std::fabs(wrap_diff(p.longitude_rad, r.lon)) < 1e-6, "longitude matches full theory");
        check(std::fabs(p.latitude_rad - r.lat) < 1e-6, "latitude matches full theory");
        check(std::fabs(p.distance_km - r.dist) < 0.2, "distance matches full theory");
    }
    // Meeus 47.a: 1992-04-12 0h TD, geometric lambda 133.162655, beta -3.229126,
    // distance 368409.7 km (his series is itself good to ~10 arcsec / 5 km).
    const ElpPosition m = elp_moon_mean_of_date(2448724.5);
    check(std::fabs(m.longitude_rad * 180 / M_PI - 133.162655) < 0.005, "Meeus 47.a longitude");
    check(std::fabs(m.latitude_rad * 180 / M_PI + 3.229126) < 0.005, "Meeus 47.a latitude");
    check(std::fabs(m.distance_km - 368409.7) < 10.0, "Meeus 47.a distance");
    // The coarse series used elsewhere agrees to its stated 0.1 degree.
    const double jde = 2460409.26;
    const MoonPosition c = moon_position(UtcSeconds{std::llround((jde - 2440587.5) * 86400.0 - 69.0)});
    const ElpPosition e = elp_moon_mean_of_date(jde);
    check(std::fabs(wrap_diff(e.longitude_rad, c.longitude_deg * M_PI / 180)) < 0.003, "agrees with coarse series");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
