// OVATION aurora port; mirrors src/widgets/solaraurora/tests (the oracle) on the same fixture,
// plus a full-size synthetic grid to keep the JSON reader fast enough for the real product.
#include "../aurora.hpp"
#include "../hcpcore.h"
#include "test_util.hpp"
#include <chrono>
#include <vector>

using namespace hcptest;

namespace {
constexpr int64_t kObserved = 1791953820000LL; // 2026-10-14T04:57:00Z
constexpr int64_t kForecast = 1791956760000LL; // 2026-10-14T05:46:00Z
constexpr double kMaxParseSeconds = 1.0;

std::string full_grid() {
    std::string s = R"({"Observation Time":"2026-10-14T04:57:00Z","Forecast Time":"2026-10-14T05:46:00Z","coordinates":[)";
    for (int lon = 0; lon < 360; ++lon)
        for (int lat = -90; lat <= 90; ++lat) {
            s += '[' + std::to_string(lon) + ',' + std::to_string(lat) + ',' + std::to_string((lon + lat + 90) % 7) + "],";
        }
    s.back() = ']';
    return s + R"(,"type":"MultiPoint"})";
}
} // namespace

int main() {
    using namespace hcp;
    const auto body = fixture("ovation_aurora_latest.json");
    check(!body.empty(), "fixture present");
    const auto g = aurora::parse(body);
    check(g.valid && g.observed_ms == kObserved, "observation time");
    check(g.forecast_ms == kForecast, "forecast time");
    check(g.at(-90 + 35, 0) >= 0 && aurora::peak(g, true) > 0 && aurora::peak(g, true) <= 100, "northern peak in range");
    check(aurora::peak(g, false) > 0, "southern peak present");

    const auto small = aurora::parse(R"({"Observation Time":"2026-10-14T04:57:00Z","Forecast Time":"2026-10-14T05:46:00Z",)"
                                     R"("coordinates":[[0,65,40],[180,-60,12],[10,95,50],[5,70,"x"],[400,10,9]]})");
    check(small.valid && small.at(65, 0) == 40 && small.at(-60, 180) == 12 && small.at(70, 5) == 0, "cells placed, bad rows skipped");
    check(aurora::peak(small, true) == 40 && aurora::peak(small, false) == 12, "peaks per hemisphere");
    check(!aurora::parse("").valid && !aurora::parse("<html>").valid && !aurora::parse("{}").valid && !aurora::parse("[]").valid,
          "garbage rejected");
    check(!aurora::parse(R"({"coordinates":[[0,65,40]]})").valid, "no observation time -> invalid");

    // C ABI, with and without the grid.
    std::vector<unsigned char> grid(360 * 181, 0xFF);
    HcpAurora a{};
    check(hcp_aurora_parse(body.data(), body.size(), &a, grid.data()) == 1 && a.observed_ms == kObserved &&
              a.forecast_ms == kForecast && a.north_peak == aurora::peak(g, true) && a.south_peak == aurora::peak(g, false),
          "C ABI parse and peaks");
    bool same = true;
    for (int lat = -90; lat <= 90; ++lat)
        for (int lon = 0; lon < 360; ++lon) same = same && grid[size_t((lat + 90) * 360 + lon)] == g.at(lat, lon);
    check(same, "C ABI grid layout (lat+90)*360+lon");
    check(hcp_aurora_parse(body.data(), body.size(), &a, nullptr) == 1, "C ABI grid optional");
    check(hcp_aurora_parse("[]", 2, &a, grid.data()) == 0 && !a.valid && a.north_peak == 0 && grid[0] == 0 && grid[65000] == 0,
          "C ABI invalid body zeroes the grid");

    const std::string big = full_grid();
    const auto t0 = std::chrono::steady_clock::now();
    const int ok = hcp_aurora_parse(big.data(), big.size(), &a, grid.data());
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::cout << "full 360x181 grid (" << big.size() / 1024 << " KiB) parsed in " << secs << " s\n";
    check(ok == 1 && a.north_peak == 6 && grid[size_t(180 * 360 + 359)] == (359 + 180) % 7, "full grid parsed");
    check(secs < kMaxParseSeconds, "full grid parses well under a second");
    return finish("aurora");
}
