// Find-in-the-sky pointing model and pass schedule: consistency against the
// SGP4 wrapper on a real ISS TLE (fixture), hand-checked compass letters and
// Doppler signs. Offline. Not compared with a published pass table.
#include "../../../domain/astro_finder.hpp"
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
    check(compass_letters(0) == "N" && compass_letters(359) == "N" && compass_letters(90) == "E"
              && compass_letters(202.5) == "SSW" && compass_letters(-90) == "W" && compass_letters(315) == "NW",
          "compass letters");
    if (argc < 2) { std::cerr << "usage: finder_regression <tle>\n"; return 2; }
    std::ifstream in(argv[1]);
    std::stringstream ss; ss << in.rdbuf();
    const Tle* iss = nullptr;
    const auto tles = parse_tle_set(ss.str());
    for (const Tle& t : tles) if (t.line1.substr(2, 5) == "25544") iss = &t;
    check(iss != nullptr, "ISS in fixture");
    if (!iss) return 1;
    SatellitePropagator sat(*iss);
    const Observer obs{51.5, -0.1, 0.0};
    const double from = sat.epoch_unix() + 3600;

    const auto passes = sat.passes(obs, from, 48 * 3600, 10.0, 10);
    check(passes.size() >= 3, "several ISS passes in 48 h");
    if (passes.empty()) return 1;
    const SatPass& p0 = passes.front();
    const FinderPass f = describe_pass(sat, obs, p0);
    check(f.duration_s == p0.los - p0.aos && f.duration_s > 60 && f.duration_s < 900, "pass duration plausible");
    check(f.aos_compass == compass_letters(f.aos_az_deg) && f.los_compass == compass_letters(f.los_az_deg),
          "compass letters follow azimuths");

    const auto at_aos = pointing_at(sat, obs, static_cast<double>(p0.aos));
    const auto at_tca = pointing_at(sat, obs, static_cast<double>(p0.tca));
    const auto at_los = pointing_at(sat, obs, static_cast<double>(p0.los));
    check(at_aos && at_tca && at_los, "pointing available");
    if (!at_aos || !at_tca || !at_los) return 1;
    check(std::fabs(at_aos->el_deg - 10.0) < 0.2 && std::fabs(at_los->el_deg - 10.0) < 0.2, "AOS/LOS at the cut-off");
    check(std::fabs(at_tca->el_deg - p0.max_el_deg) < 0.5 && at_tca->el_deg >= at_aos->el_deg, "TCA at max elevation");
    check(at_aos->range_rate_km_s < 0 && at_los->range_rate_km_s > 0 && std::fabs(at_tca->range_rate_km_s) < 1.0,
          "range rate: approaching, ~0 at TCA, receding");
    check(at_aos->range_km > at_tca->range_km && at_los->range_km > at_tca->range_km, "closest at TCA");
    check(at_tca->above_horizon, "above horizon at TCA");

    // Doppler: approaching raises the downlink and lowers the uplink to transmit.
    const DopplerPair d = doppler_for(*at_aos, 145.8e6, 435.0e6);
    check(d.downlink_hz > 145.8e6 && d.uplink_hz < 435.0e6, "Doppler sense at AOS");
    check(std::fabs(d.downlink_hz - doppler_downlink_hz(145.8e6, at_aos->range_rate_km_s)) < 1e-6, "uses existing formula");
    const double shift = d.downlink_hz - 145.8e6;
    check(shift > 1500 && shift < 4000, "2 m ISS shift ~ +3 kHz at AOS");

    // Rise point before AOS: horizon crossing azimuth matches the 0 deg pass.
    const auto rise = rise_point(sat, obs, from, 48 * 3600);
    check(rise.has_value(), "rise point found");
    if (rise) {
        const auto at_rise = pointing_at(sat, obs, static_cast<double>(rise->t));
        check(at_rise && std::fabs(at_rise->el_deg) < 0.2 && std::fabs(at_rise->az_deg - rise->az_deg) < 0.5,
              "rise point is on the horizon at that azimuth");
        check(rise->t <= p0.aos, "horizon rise is no later than the 10 deg AOS");
    }

    // The pass search leaps over the hours far below the horizon; a brute-force 5 s scan of the same
    // 48 h must find the same passes (same count, AOS within a scan step).
    {
        const auto fast = sat.passes(obs, from, 48 * 3600, 0.0, 64);
        std::vector<double> brute_aos;
        bool was_up = false;
        for (double t = from; t <= from + 48 * 3600; t += 5.0) {
            LookAngle la;
            const bool up = sat.look(t, obs, la) && la.el_deg >= 0.0;
            if (up && !was_up) brute_aos.push_back(t);
            was_up = up;
        }
        bool same = fast.size() == brute_aos.size();
        for (std::size_t i = 0; same && i < fast.size(); ++i) same = std::fabs(static_cast<double>(fast[i].aos) - brute_aos[i]) <= 6.0;
        check(same, "leaping pass search matches a 5 s brute-force scan");
    }

    // Schedule: sorted, filter only removes, visible ones satisfy the rule.
    const std::vector<std::pair<std::string, const SatellitePropagator*>> sats{{"ISS", &sat}};
    const auto all = pass_schedule(sats, obs, from, 48 * 3600, 10.0, PassFilter::All, 0);
    const auto vis = pass_schedule(sats, obs, from, 48 * 3600, 10.0, PassFilter::VisibleToEye, 0);
    bool sorted = true;
    for (std::size_t i = 1; i < all.size(); ++i) sorted = sorted && all[i - 1].pass.pass.aos <= all[i].pass.pass.aos;
    check(sorted && all.size() == passes.size(), "schedule sorted, matches pass search");
    check(vis.size() <= all.size(), "visible filter only removes");
    bool dark = true;
    for (const auto& s : vis) dark = dark && s.pass.sun_el_at_tca_deg < 5.0 && s.pass.visible_to_eye;
    check(dark, "visible passes happen near/after dusk or before dawn");
    check(pass_schedule(sats, obs, from, 48 * 3600, 10.0, PassFilter::All, 2).size() == 2, "max_passes caps");

    const std::vector<std::pair<std::string, const SatellitePropagator*>> two{{"ISS", &sat}, {"OTHER", &sat}};
    const auto only = pass_schedule(two, obs, from, 48 * 3600, 10.0, PassFilter::All, 0,
                                    [](const std::string& n) { return n == "OTHER"; });
    bool named = !only.empty() && only.size() == all.size();
    for (const auto& s : only) named = named && s.name == "OTHER";
    check(named, "include predicate keeps only accepted satellites");
    check(pass_schedule(two, obs, from, 48 * 3600, 10.0, PassFilter::All, 0,
                        [](const std::string&) { return false; }).empty(), "include rejecting all gives none");

    // Magnitude: ISS-class only, bright and plausible only for dark-sky sunlit passes.
    check(standard_magnitude("ISS (ZARYA)") && !standard_magnitude("AO-91") && standard_magnitude("CSS (TIANHE)"),
          "standard magnitude only for known large objects");
    int mags = 0;
    bool plausible = true;
    for (const auto& s : vis) {
        for (double t = s.pass.pass.aos; t <= s.pass.pass.los; t += 15.0) {
            const auto m = estimate_magnitude(sat, obs, t, -1.8);
            if (m) { ++mags; plausible = plausible && *m > -6.0 && *m < 5.0; }
        }
    }
    check(vis.empty() || (mags > 0 && plausible), "magnitudes exist and are plausible on visible passes");
    // Daylight pass: no magnitude.
    bool day_none = true;
    for (const auto& s : all)
        if (s.pass.sun_el_at_tca_deg > 10.0) day_none = day_none && !estimate_magnitude(sat, obs, s.pass.pass.tca, -1.8);
    check(day_none, "no magnitude in daylight");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
