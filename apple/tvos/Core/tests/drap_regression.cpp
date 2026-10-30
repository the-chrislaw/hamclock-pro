// D-RAP port; mirrors src/widgets/solardrap/tests (the oracle) on the same fixture.
#include "../drap.hpp"
#include "../hcpcore.h"
#include "test_util.hpp"
#include <string>
#include <vector>

using namespace hcptest;

int main() {
    using namespace hcp;
    constexpr int64_t kValidAt = 1791958020000LL; // 2026-10-14T06:07:00Z
    const auto body = fixture("drap_global_frequencies.txt");
    check(!body.empty(), "fixture present");
    const auto g = drap::parse(body);
    check(g.valid && g.valid_ms == kValidAt, "valid time");
    check(g.xray == "Normal X-ray Background" && g.proton == "Normal Proton Background", "messages");
    check(g.mhz.size() == 90u * 90u, "full grid");
    check(drap::peak(g) > 3.9 && drap::peak(g) < 4.1, "peak 4.0 MHz");
    check(g.mhz[0] == 0.0f, "north pole row is zero");
    const std::string cut = body.substr(0, body.size() / 2);
    check(!drap::parse(cut).valid && !drap::parse("").valid && !drap::parse("<html>").valid, "truncated or garbage rejected");

    std::vector<float> grid(90 * 90, -1.0f);
    HcpDrap d{};
    check(hcp_drap_parse(body.data(), body.size(), &d, grid.data()) == 1 && d.valid_ms == kValidAt &&
              near(d.peak_mhz, drap::peak(g), 1e-9), "C ABI parse");
    check(std::string(d.xray) == "Normal X-ray Background" && std::string(d.proton) == "Normal Proton Background", "C ABI messages");
    check(grid[0] == 0.0f && grid[90 * 90 - 1] == g.mhz.back() && grid[45 * 90 + 45] == g.mhz[45 * 90 + 45], "C ABI grid row-major");
    check(hcp_drap_parse(body.data(), body.size(), &d, nullptr) == 1, "C ABI grid optional");
    check(hcp_drap_parse(cut.data(), cut.size(), &d, grid.data()) == 0 && !d.valid && d.peak_mhz == 0 && grid[100] == 0.0f,
          "C ABI rejects truncated table and zeroes the grid");
    return finish("drap");
}
