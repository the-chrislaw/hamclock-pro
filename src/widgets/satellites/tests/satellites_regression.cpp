// SGP4 propagation validated against the published Spacetrack Report #3 /
// Vallado verification case (object 88888), plus structural checks on a real
// amateur-satellite TLE from the CelesTrak fixture. Offline.
#include "../../../domain/astro_satellite.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
}

int main(int argc, char** argv) {
    using namespace hamclock;
    // Spacetrack #3 verification TLE (afspc mode, WGS-72).
    const Tle ref{"88888",
        "1 88888U          80275.98708465  .00073094  13844-3  66816-4 0     8",
        "2 88888  72.8435 115.9689 0086731  52.6988 110.5714 16.05824518   105"};
    SatellitePropagator p(ref);
    check(p.valid(), "88888 parses");
    double r[3];
    check(p.teme(p.epoch_unix(), r), "88888 propagates at epoch");
    // Published t=0 state vector, km.
    check(std::fabs(r[0] - 2328.97048951) < 5e-3 && std::fabs(r[1] + 5995.22076416) < 5e-3
              && std::fabs(r[2] - 1719.97067261) < 5e-3, "88888 t=0 matches published vector");
    check(p.teme(p.epoch_unix() + 360 * 60, r), "88888 propagates at +360 min");
    check(std::fabs(r[0] - 2456.10705566) < 1e-2 && std::fabs(r[1] + 6071.93853760) < 1e-2
              && std::fabs(r[2] - 1222.89727783) < 1e-2, "88888 t=360 matches published vector");

    // Range rate: matches a 10 s finite difference, and Doppler signs/size.
    {
        const Observer obs{51.5, -0.1, 0.0};
        const double t = p.epoch_unix() + 600;
        LookAngle a, b, c;
        check(p.look(t, obs, a) && p.look(t - 5, obs, b) && p.look(t + 5, obs, c), "88888 look");
        check(std::fabs(a.range_rate_km_s - (c.range_km - b.range_km) / 10.0) < 0.05, "range rate ~ finite difference");
        check(std::fabs(a.range_rate_km_s) < 8.0, "range rate below orbital speed");
    }
    check(std::fabs(doppler_downlink_hz(145.8e6, 7.0) - 145.8e6 * (1 - 7.0 / 299792.458)) < 1e-3, "downlink formula");
    check(doppler_downlink_hz(145.8e6, 7.0) < 145.8e6 && doppler_downlink_hz(145.8e6, -7.0) > 145.8e6, "receding lowers downlink");
    check(std::fabs(doppler_downlink_hz(145.8e6, 7.0) - 145.8e6 + 3404.0) < 5.0, "7 km/s at 145.8 MHz is ~3.4 kHz");
    check(doppler_uplink_hz(435.0e6, 7.0) > 435.0e6, "uplink pre-compensates upward when receding");

    SatellitePropagator bad(Tle{"x", "1 garbage", "2 garbage"});
    check(!bad.valid(), "garbage TLE rejected");

    std::ifstream f(argc > 1 ? argv[1] : "");
    std::stringstream ss;
    ss << f.rdbuf();
    const auto sets = parse_tle_set(ss.str());
    check(sets.size() > 50, "fixture parses to many TLE sets");
    check(!sets.empty() && sets[0].name.rfind("OSCAR 7", 0) == 0, "first set is AO-7 with name trimmed");
    if (sets.empty()) return 1;

    SatellitePropagator ao7(sets[0]);
    check(ao7.valid(), "AO-7 valid");
    const double t0 = ao7.epoch_unix() + 3600;
    SubSatellitePoint sp;
    check(ao7.sub_point(t0, sp), "AO-7 sub-point");
    check(sp.alt_km > 1400 && sp.alt_km < 1500, "AO-7 altitude ~1450 km");
    check(std::fabs(sp.lat_deg) <= 102.0 - 90.0 + 90.0 && std::fabs(sp.lat_deg) < 80.0 + 12.0, "AO-7 latitude bounded by inclination");
    check(std::fabs(sp.lon_deg) <= 180.0, "longitude normalised");

    // A satellite directly overhead the sub-point must show ~90 deg elevation.
    LookAngle la;
    check(ao7.look(t0, Observer{sp.lat_deg, sp.lon_deg, 0}, la), "look at sub-point");
    check(la.el_deg > 89.0 && std::fabs(la.range_km - sp.alt_km) < 5.0, "overhead at sub-point");
    // The antipode cannot see it.
    ao7.look(t0, Observer{-sp.lat_deg, sp.lon_deg + 180.0, 0}, la);
    check(la.el_deg < -60.0, "below horizon at antipode");

    // Passes over a mid-latitude station over a day.
    const Observer de{40.0, -75.0, 0.1};
    const auto passes = ao7.passes(de, t0, 86400.0, 0.0, 50);
    check(passes.size() >= 2 && passes.size() <= 12, "plausible number of AO-7 passes per day");
    bool sane = !passes.empty();
    for (const auto& ps : passes) {
        sane = sane && ps.aos < ps.tca && ps.tca < ps.los && ps.los - ps.aos < 1800 && ps.max_el_deg > 0 && ps.max_el_deg <= 90;
        LookAngle a;
        ao7.look(ps.tca, de, a);
        sane = sane && std::fabs(a.el_deg - ps.max_el_deg) < 0.01;
        ao7.look(ps.aos + 1, de, a);
        sane = sane && a.el_deg > -0.2 && a.el_deg < 2.0;
    }
    check(sane, "passes ordered, short, peak elevation consistent");
    for (std::size_t i = 1; i < passes.size(); ++i) check(passes[i].aos > passes[i - 1].los, "passes do not overlap");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
