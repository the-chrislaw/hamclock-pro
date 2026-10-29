// Kp / X-ray / solar-wind history through the C ABI; the last point must equal the Qt oracle's
// latest value (src/widgets/spaceweather/tests, src/widgets/solarwind/tests) on the same fixtures.
#include "../hcpcore.h"
#include "test_util.hpp"
#include <vector>

using namespace hcptest;

namespace {
constexpr int64_t kOct13 = 1791849600000LL; // 2026-10-13T00:00:00Z

bool oldest_first(const std::vector<HcpReading>& v, int n) {
    for (int i = 1; i < n; ++i)
        if (v[size_t(i - 1)].time_ms > v[size_t(i)].time_ms) return false;
    return true;
}
} // namespace

int main() {
    std::vector<HcpReading> out(4096);
    const auto kp = fixture("noaa-planetary-k-index.json");
    const int nk = hcp_swpc_kp_series(kp.data(), kp.size(), out.data(), out.size());
    check(nk > 1 && oldest_first(out, nk), "Kp series oldest first");
    check(nk > 0 && out[size_t(nk - 1)].value == 3.0 && out[size_t(nk - 1)].time_ms == kOct13 + 12 * 3600000LL,
          "Kp series ends at the oracle's latest (3.0 at 12:00Z)");
    check(nk > 0 && out[0].value == 1.33 && out[0].valid, "Kp series starts with the oldest row");
    HcpReading two[2];
    check(hcp_swpc_kp_series(kp.data(), kp.size(), two, 2) == 2 && two[1].value == 3.0 &&
              two[0].time_ms < two[1].time_ms, "cap keeps the most recent readings");
    check(hcp_swpc_kp_series("{}", 2, out.data(), out.size()) == -1 && hcp_swpc_kp_series(nullptr, 0, out.data(), 4) == -1,
          "not the product -> -1");
    const char* mixed = R"([{"time_tag":"2026-10-13T03:00:00","Kp":12},{"time_tag":"x","Kp":2},{"time_tag":"2026-10-13T00:00:00","Kp":2}])";
    check(hcp_swpc_kp_series(mixed, std::strlen(mixed), out.data(), out.size()) == 1 && out[0].value == 2,
          "invalid rows skipped");

    const auto xr = fixture("xrays-6-hour.json");
    const int nx = hcp_swpc_xray_series(xr.data(), xr.size(), out.data(), out.size());
    check(nx > 1 && oldest_first(out, nx), "X-ray series oldest first");
    check(nx > 0 && near(out[size_t(nx - 1)].value, 7.258945515786763e-07, 1e-15), "X-ray series ends at the 0.1-0.8 nm latest");
    const char* band = R"([{"time_tag":"2026-10-13T00:00:00Z","flux":1e-6,"energy":"0.05-0.4nm"}])";
    check(hcp_swpc_xray_series(band, std::strlen(band), out.data(), out.size()) == 0, "short channel ignored");

    const auto wind = widget_fixture("solarwind", "rtsw_wind_1m.json");
    const auto mag = widget_fixture("solarwind", "rtsw_mag_1m.json");
    const int ns = hcp_solarwind_series(wind.data(), wind.size(), 0, out.data(), out.size());
    check(ns == 10 && oldest_first(out, ns), "10 active speed rows (backup spacecraft ignored)");
    check(ns == 10 && out[9].value == 565.6 && out[9].time_ms == kOct13 + (17 * 60 + 38) * 60000LL, "latest active speed");
    check(hcp_solarwind_series(wind.data(), wind.size(), 1, out.data(), out.size()) > 0, "density series");
    check(hcp_solarwind_series(mag.data(), mag.size(), 2, out.data(), out.size()) > 0, "Bt series");
    const int nz = hcp_solarwind_series(mag.data(), mag.size(), 3, out.data(), out.size());
    check(nz > 0 && out[size_t(nz - 1)].value == 1.42, "Bz series ends at the latest GSM value");
    check(hcp_solarwind_series(mag.data(), mag.size(), 4, out.data(), out.size()) == -1 &&
              hcp_solarwind_series("garbage", 7, 0, out.data(), out.size()) == -1, "bad field or body -> -1");
    return finish("series");
}
