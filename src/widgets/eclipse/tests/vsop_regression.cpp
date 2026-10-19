// Truncated VSOP87D Earth against Meeus, Astronomical Algorithms ch. 25,
// example 25.b (1992 Oct 13, 0h TD): geocentric Sun true longitude
// 199.907372 deg (VSOP87, before the FK5 correction), R = 0.99760775 AU.
#include "../../../domain/astro_vsop.hpp"
#include <cmath>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
}

int main() {
    const hamclock::EarthHelio e = hamclock::vsop87d_earth(2448908.5);
    const double lon_deg = e.longitude_rad * 180.0 / M_PI;
    std::cout.precision(10); std::cout << "L " << lon_deg << " B " << e.latitude_rad << " R " << e.radius_au << '\n';
    check(std::fabs(lon_deg - 19.907372) < 2e-5, "heliocentric longitude (0.07 arcsec)");
    check(std::fabs(e.radius_au - 0.99760775) < 1e-6, "radius vector (truncation leaves ~5e-7 AU)");
    check(std::fabs(e.latitude_rad) < 1e-5, "latitude under 2 arcsec");
    check(e.longitude_rad >= 0 && e.longitude_rad < 2 * M_PI, "longitude in range");
    // Perihelion/aphelion radius bounds over a year.
    double lo = 9, hi = 0;
    for (int d = 0; d < 366; ++d) { const double r = hamclock::vsop87d_earth(2460311.5 + d).radius_au; lo = std::fmin(lo, r); hi = std::fmax(hi, r); }
    check(lo > 0.9832 && lo < 0.9834 && hi > 1.0166 && hi < 1.0168, "annual radius range");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
