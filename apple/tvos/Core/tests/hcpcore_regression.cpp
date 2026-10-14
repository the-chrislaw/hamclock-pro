// C ABI smoke test: same NOAA fixtures as swpc_regression, through hcpcore.h.
#include "../hcpcore.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static int failures = 0;
static void check(bool ok, const char* label) { if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; } }
static std::string fixture(const char* name) {
    std::ifstream in(std::string(HCP_FIXTURE_DIR) + "/" + name, std::ios::binary);
    std::stringstream ss; ss << in.rdbuf(); return ss.str();
}

int main() {
    const std::string kp = fixture("noaa-planetary-k-index.json");
    const HcpReading k = hcp_swpc_latest_kp(kp.data(), kp.size());
    check(k.valid && k.value == 3.0 && k.time_ms == 1791892800000LL, "kp");
    const std::string fl = fixture("10cm-flux.json");
    const HcpReading f = hcp_swpc_latest_solar_flux(fl.data(), fl.size());
    check(f.valid && f.value == 123, "flux");
    const std::string xr = fixture("xrays-6-hour.json");
    const HcpReading x = hcp_swpc_latest_xray(xr.data(), xr.size());
    char buf[16];
    check(x.valid && hcp_swpc_xray_class(x.value, buf, sizeof buf) && std::string(buf) == "B7.2", "xray class");
    check(!hcp_swpc_latest_kp("garbage", 7).valid && !hcp_swpc_latest_kp(nullptr, 0).valid, "bad input");
    check(hcp_swpc_xray_class(3.2e-6, buf, 3) && std::string(buf) == "C3", "truncates safely");
    check(!hcp_swpc_xray_class(-1, buf, sizeof buf), "invalid flux -> empty");
    check(hcp_swpc_kp_description(3, buf, sizeof buf) >= 0, "description callable");
    return failures ? 1 : 0;
}
