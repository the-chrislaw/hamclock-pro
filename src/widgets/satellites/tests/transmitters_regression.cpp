// SatNOGS transmitter parsing, NORAD join and transponder maths, against a
// trimmed real API response (tests/fixtures/sources/satnogs-transmitters.json).
#include "../../../domain/astro_transmitters.hpp"
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
std::string slurp(const char* path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
}

int main(int argc, char** argv) {
    using namespace hamclock;
    if (argc < 3) { std::cerr << "usage: fixture.json amateur.tle\n"; return 2; }
    const auto all = parse_satnogs_transmitters(slurp(argv[1]));
    check(all.size() == 10, "fixture has ten transmitters");
    const auto by = group_by_norad(all);
    check(by.count(7530) && by.at(7530).size() == 3, "AO-7 has three");
    check(by.count(25544) && by.at(25544).size() == 2, "ISS has two");

    // Single-frequency FM voice with a CTCSS tone (SO-50).
    const Transmitter* fm = nullptr;
    for (const auto& t : by.at(27607)) if (t.ctcss_hz > 0) fm = &t;
    check(fm && std::fabs(fm->ctcss_hz - 67.0) < 1e-9 && fm->mode == "FM", "SO-50 FM voice CTCSS 67.0");
    check(fm && fm->downlink_low_hz == 436795000 && fm->uplink_low_hz == 145850000 && !fm->transponder(),
          "SO-50 frequencies");

    // Non-inverting AO-7 mode A: 145.850 up -> 29.400 down (passband edge maps to edge).
    const Transmitter* ao7 = nullptr;
    for (const auto& t : by.at(7530)) if (t.transponder()) ao7 = &t;
    check(ao7 && !ao7->inverting, "AO-7 mode A linear, not inverting");
    check(ao7 && transponder_downlink_hz(*ao7, ao7->uplink_low_hz) == ao7->downlink_low_hz, "AO-7 low edge");
    check(ao7 && transponder_downlink_hz(*ao7, ao7->uplink_low_hz + 25000) == ao7->downlink_low_hz + 25000,
          "AO-7 offset follows uplink");
    check(ao7 && transponder_downlink_hz(*ao7, ao7->uplink_high_hz + 1) == 0, "outside passband gives 0");

    // Inverting FO-29: top of the uplink passband maps to the bottom of the downlink.
    const Transmitter* fo = nullptr;
    for (const auto& t : by.at(24278)) if (t.inverting) fo = &t;
    check(fo != nullptr, "FO-29 inverting transponder");
    if (fo) {
        check(transponder_downlink_hz(*fo, fo->uplink_high_hz) == fo->downlink_low_hz, "inverting: uplink top -> downlink bottom");
        check(transponder_downlink_hz(*fo, fo->uplink_low_hz + 30000) == fo->downlink_high_hz - 30000
                  || transponder_downlink_hz(*fo, fo->uplink_low_hz + 30000) == fo->downlink_low_hz + (fo->uplink_high_hz - fo->uplink_low_hz - 30000),
              "inverting: offset reverses");
    }

    // NORAD join against the real CelesTrak amateur set: AO-7 is 7530 and leading zeros are handled.
    const auto tles = parse_tle_set(slurp(argv[2]));
    int ao7_id = 0, joined = 0;
    for (const auto& tle : tles) {
        const int id = tle_norad_id(tle);
        if (tle.name.rfind("OSCAR 7", 0) == 0) ao7_id = id;
        if (by.count(id)) ++joined;
    }
    check(ao7_id == 7530, "AO-7 NORAD id from TLE");
    check(joined >= 3, "TLEs join to transmitters by NORAD id");
    check(tle_norad_id(Tle{"x", "1", "2"}) == 0, "short line gives 0");
    check(parse_satnogs_transmitters("not json").empty() && parse_satnogs_transmitters("{}").empty(), "malformed input gives none");

    // User-supplied TLEs replace the same NORAD id and append new ones.
    {
        const Tle a{"A", "1 00005U 58002B   00179.78495062  .00000023  00000-0  28098-4 0  4753", "2 00005"};
        const Tle b{"B", "1 00006U 58002B   00179.78495062  .00000023  00000-0  28098-4 0  4753", "2 00006"};
        Tle a2 = a; a2.name = "A-user";
        const Tle c{"C", "1 00007U 58002B   00179.78495062  .00000023  00000-0  28098-4 0  4753", "2 00007"};
        const auto m = merge_user_tles({a, b}, {a2, c});
        check(m.size() == 3 && m[0].name == "A-user" && m[1].name == "B" && m[2].name == "C", "user TLE replaces by NORAD id and appends");
    }

    // Doppler through an inverting transponder: the uplink correction and downlink
    // correction at the same range rate move in opposite senses.
    if (fo) {
        const double rr = 5.0;
        check(doppler_downlink_hz(fo->downlink_low_hz, rr) < fo->downlink_low_hz
                  && doppler_uplink_hz(fo->uplink_low_hz, rr) > fo->uplink_low_hz, "receding: down lower, up higher");
    }
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
