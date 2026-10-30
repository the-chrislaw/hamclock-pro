// Batched satellite propagation (hcp_sat_states) against the per-index C ABI on the 95-satellite
// CelesTrak amateur fixture. The batched call parses the TLE text once; the per-index calls re-parse
// it for every satellite. Timings are printed (not asserted) as evidence for the tvOS stutter fix.
#include "../hcpcore.h"
#include "test_util.hpp"
#include <chrono>
#include <vector>

using hcptest::check;

namespace {
double ms_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}
} // namespace

int main() {
    const std::string tle = hcptest::fixture("celestrak-amateur.tle");
    const char* p = tle.data();
    const size_t len = tle.size();
    const int64_t when = 1792000000;        // 2026-10-15, a few days after the fixture's epochs
    const double lat = 41.71, lon = -72.73; // FN31pr (W1AW)

    const int n = hcp_tle_count(p, len);
    check(n >= 90, "fixture holds the amateur set");
    const size_t count = static_cast<size_t>(n > 0 ? n : 0);

    auto t0 = std::chrono::steady_clock::now();
    std::vector<HcpSatState> old_states(count);
    std::vector<int> old_ok(count);
    std::vector<std::string> old_names(count);
    for (size_t i = 0; i < count; ++i) {
        old_ok[i] = hcp_sat_state(p, len, static_cast<int>(i), when, lat, lon, &old_states[i]);
        char name[32] = {};
        hcp_tle_name(p, len, static_cast<int>(i), name, sizeof name);
        old_names[i] = name;
    }
    const double per_index_ms = ms_since(t0);

    t0 = std::chrono::steady_clock::now();
    std::vector<HcpSatInfo> batch(count);
    const int m = hcp_sat_states(p, len, when, lat, lon, batch.data(), batch.size());
    const double batch_ms = ms_since(t0);

    check(m == n, "batch reports every set");
    bool same = m == n;
    size_t valid = 0;
    for (size_t i = 0; same && i < count; ++i) {
        const auto& b = batch[i];
        const auto& o = old_states[i];
        same = b.valid == old_ok[i] && old_names[i] == b.name &&
               (!b.valid || (b.state.lat_deg == o.lat_deg && b.state.lon_deg == o.lon_deg &&
                             b.state.el_deg == o.el_deg && b.state.az_deg == o.az_deg &&
                             b.state.epoch_s == o.epoch_s && b.state.sunlit == o.sunlit));
        valid += b.valid ? 1 : 0;
    }
    check(same, "batch equals the per-index states and names");
    check(valid > count / 2, "most satellites propagate");

    std::vector<HcpSatInfo> few(3);
    check(hcp_sat_states(p, len, when, lat, lon, few.data(), few.size()) == n && few[2].name[0] != 0,
          "cap limits what is written, not the count");
    check(hcp_sat_states(p, len, when, 95.0, lon, batch.data(), batch.size()) == 0, "bad latitude rejected");
    check(hcp_sat_states(nullptr, 0, when, lat, lon, batch.data(), batch.size()) == 0, "null text rejected");
    check(hcp_sat_states("garbage", 7, when, lat, lon, batch.data(), batch.size()) == 0, "garbage has no sets");

    std::cout << "satellites: " << n << ", per-index ABI " << per_index_ms << " ms, batched " << batch_ms << " ms\n";
    return hcptest::finish("satbatch");
}
