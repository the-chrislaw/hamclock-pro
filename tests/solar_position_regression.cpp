#include "../src/domain/solar_position.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
int main() {
    using namespace hamclock;
    int failures = 0;
    auto near = [&](double a, double b, double tol, const char* name) {
        if (!(std::fabs(a - b) <= tol)) { ++failures; std::cerr << "Solar regression failed: " << name << " got " << a << " want " << b << '\n'; }
    };
    // Meeus, Astronomical Algorithms 2nd ed., example 25.a: 1992-10-13 00:00 TD
    // (JD 2448908.5 = Unix 718934400): apparent declination -7.78507 degrees.
    const auto meeus = subsolar_point({718934400});
    near(meeus.latitude_deg, -7.78507, 0.02, "Meeus 25.a declination");
    // Equation of time that day is +13m 42.6s, so the Sun is overhead at
    // longitude -(0h-12h)*15 - 13.71min/4 = 180 - 3.43 = 176.57 degrees.
    near(meeus.longitude_deg, 176.57, 0.05, "Meeus 25.a subsolar longitude");
    // 2024 March equinox 03:06 UTC (Unix 1710904000 approx): declination ~ 0.
    near(subsolar_point({1710904000}).latitude_deg, 0.0, 0.05, "March equinox 2024 declination");
    // 2024 June solstice 20:51 UTC (Unix 1718916660): declination ~ +23.44.
    near(subsolar_point({1718916660}).latitude_deg, 23.44, 0.02, "June solstice 2024 declination");
    // 2024 December solstice 09:20 UTC (Unix 1734772800): declination ~ -23.44.
    near(subsolar_point({1734772800}).latitude_deg, -23.44, 0.02, "December solstice 2024 declination");
    // Local solar noon at Greenwich around 12:00 UTC in mid-April (EoT ~ 0).
    near(subsolar_point({1713182400}).longitude_deg, 0.0, 1.0, "mid-April noon subsolar near Greenwich");
    const SubsolarPoint sun{20.0, 30.0};
    near(solar_elevation_deg(sun, 20.0, 30.0), 90.0, 1e-9, "elevation at subsolar point");
    near(solar_elevation_deg(sun, -20.0, -150.0), -90.0, 1e-9, "elevation at antipode");
    near(solar_elevation_deg(sun, terminator_latitude_deg(sun, 100.0), 100.0), 0.0, 1e-9, "terminator elevation zero");
    near(solar_elevation_deg(sun, terminator_latitude_deg(sun, -77.0), -77.0), 0.0, 1e-9, "terminator elevation zero west");
    near(terminator_latitude_deg(sun, 120.0), 0.0, 1e-9, "terminator crosses equator 90 degrees from subsolar");
    near(terminator_latitude_deg(sun, 30.0), -70.0, 1e-9, "terminator under subsolar meridian is 90-dec south");
    near(terminator_latitude_deg(sun, 210.0 - 360.0), 70.0, 1e-9, "terminator on antisolar meridian is 90-dec north");
    if (!std::isfinite(terminator_latitude_deg({0.0, 0.0}, 45.0))) { ++failures; std::cerr << "equinox terminator not finite\n"; }
    bool threw = false;
    try { solar_elevation_deg(sun, 91.0, 0.0); } catch (const std::invalid_argument&) { threw = true; }
    if (!threw) { ++failures; std::cerr << "invalid latitude accepted\n"; }
    return failures == 0 ? 0 : 1;
}
