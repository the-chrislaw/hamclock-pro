// Qt-free solar wind parser; mirrors src/widgets/solarwind/tests (the oracle) on the same fixtures.
#include "../hcpcore.h"
#include "../solarwind.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <tuple>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
std::string fixture(const char* name) {
    std::ifstream in(std::string(HCP_FIXTURE_DIR) + "/" + name, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
} // namespace

int main() {
    using namespace hcp;
    const auto wind = fixture("rtsw_wind_1m.json");
    const auto mag = fixture("rtsw_mag_1m.json");
    check(!wind.empty() && !mag.empty(), "fixtures present");

    const auto speed = solarwind::series(wind, solarwind::kSpeed);
    check(speed.size() == 10, "10 active rows (backup spacecraft ignored)");
    bool sorted = true;
    for (size_t i = 1; i < speed.size(); ++i) sorted = sorted && speed[i - 1].time_ms < speed[i].time_ms;
    check(sorted, "chronological, no duplicate minutes");
    // 2026-10-13T17:38:00Z
    check(!speed.empty() && speed.back().time_ms == 1791892800000LL + (5 * 60 + 38) * 60000LL, "latest time is UTC");
    check(!speed.empty() && speed.back().value == 565.6, "latest active speed");
    const auto bz = solarwind::latest(mag, solarwind::kBz);
    check(bz && bz->value == 1.42, "latest Bz is GSM");
    check(solarwind::latest(mag, solarwind::kBt) && solarwind::latest(wind, solarwind::kDensity), "Bt, density");

    for (const char* bad : {"", "{}", "[", "null", "[1,2]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"active\":true,\"bt\":null}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"active\":true,\"bt\":\"5\"}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"active\":true,\"bt\":-9999}]",
                            "[{\"time_tag\":\"bogus\",\"active\":true,\"bt\":5}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"active\":false,\"bt\":5}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"bt\":5}]"})
        check(solarwind::series(bad, solarwind::kBt).empty(), "malformed row rejected");

    check(solarwind::speed_description(350) == "Slow" && solarwind::speed_description(450) == "Moderate" &&
          solarwind::speed_description(650) == "Fast" && solarwind::speed_description(900) == "Very fast" &&
          solarwind::speed_description(-1).empty(), "speed descriptions");
    check(solarwind::bz_description(2) == "Northward" && solarwind::bz_description(-1) == "Near neutral" &&
          solarwind::bz_description(-5) == "Weakly south" && solarwind::bz_description(-10) == "Southward" &&
          solarwind::bz_description(-20) == "Strongly south", "Bz descriptions");

    // Through the C ABI used by Swift.
    const HcpReading s = hcp_solarwind_latest_speed(wind.data(), wind.size());
    const HcpReading z = hcp_solarwind_latest_bz(mag.data(), mag.size());
    char buf[32];
    check(s.valid && s.value == 565.6 && hcp_solarwind_speed_description(s.value, buf, sizeof buf) &&
          std::string(buf) == "Moderate", "C ABI speed");
    check(z.valid && z.value == 1.42 && hcp_solarwind_bz_description(z.value, buf, sizeof buf) &&
          std::string(buf) == "Northward", "C ABI Bz");
    check(!hcp_solarwind_latest_bt("garbage", 7).valid && !hcp_solarwind_latest_bt(nullptr, 0).valid, "C ABI bad input");

    // One parse for both fields of a body gives exactly what two single-field calls give.
    for (const auto& [body, fa, fb] : {std::tuple{wind, 0, 1}, std::tuple{mag, 2, 3}}) {
        HcpReading a1[240], b1[240], a2[240], b2[240];
        const int na1 = hcp_solarwind_series(body.data(), body.size(), fa, a1, 240);
        const int nb1 = hcp_solarwind_series(body.data(), body.size(), fb, b1, 240);
        int nb2 = 0;
        const int na2 = hcp_solarwind_series_pair(body.data(), body.size(), fa, fb, a2, b2, 240, &nb2);
        bool same = na1 > 0 && nb1 > 0 && na1 == na2 && nb1 == nb2;
        for (int i = 0; same && i < na1; ++i) same = a1[i].value == a2[i].value && a1[i].time_ms == a2[i].time_ms;
        for (int i = 0; same && i < nb1; ++i) same = b1[i].value == b2[i].value && b1[i].time_ms == b2[i].time_ms;
        check(same, "pair parse equals two single-field parses");
        HcpReading small_a[3], small_b[3];
        int nsmall = 0;
        check(na1 >= 3 && nb1 >= 3 &&
                  hcp_solarwind_series_pair(body.data(), body.size(), fa, fb, small_a, small_b, 3, &nsmall) == 3 &&
                  nsmall == 3 && small_a[2].time_ms == a1[na1 - 1].time_ms,
              "pair keeps the most recent cap rows");
    }
    int nbad = 0;
    check(hcp_solarwind_series_pair("garbage", 7, 0, 1, nullptr, nullptr, 0, &nbad) == -1 && nbad == -1,
          "pair rejects a non-product body");
    check(hcp_solarwind_series_pair(wind.data(), wind.size(), 0, 7, nullptr, nullptr, 0, &nbad) == -1 && nbad == -1,
          "pair rejects an unknown field");
    return failures ? 1 : 0;
}
