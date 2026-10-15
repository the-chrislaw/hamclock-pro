// C ABI smoke test: same NOAA fixtures as swpc_regression, through hcpcore.h.
#include "../hcpcore.h"
#include <fstream>
#include <cmath>
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
    // March equinox 2026 (2026-03-20 14:46 UTC): Sun overhead near the equator.
    double la = 99, lo = 999, el = 99;
    check(hcp_subsolar_point(1774017960LL, &la, &lo) && std::abs(la) < 0.5, "equinox declination");
    check(hcp_sun_elevation(1774017960LL, la, lo, &el) && el > 89.0, "sun overhead at subsolar point");
    check(hcp_sun_elevation(1774017960LL, la, lo + 180.0, &el) && el < -89.0, "antipode is night");
    check(!hcp_sun_elevation(0, 95.0, 0, &el), "invalid latitude rejected");
    check(!hcp_subsolar_point(0, nullptr, &lo), "null out rejected");
    // Maidenhead centres: JN58 spans 48-49N, 10-12E; FN31pr is near Newington CT.
    double cl = 0, co = 0;
    check(hcp_locator_center("JN58", &cl, &co) && std::abs(cl - 48.5) < 1e-9 && std::abs(co - 11.0) < 1e-9, "JN58 centre");
    check(hcp_locator_center("jn58td", &cl, &co) && std::abs(cl - 48.1458333) < 1e-4 && std::abs(co - 11.625) < 1e-4, "JN58td centre");
    check(hcp_locator_center("JN", &cl, &co) && std::abs(cl - 45.0) < 1e-9 && std::abs(co - 10.0) < 1e-9, "JN field centre");
    check(!hcp_locator_center("ZZ99", &cl, &co) && !hcp_locator_center("JN5", &cl, &co) && !hcp_locator_center(nullptr, &cl, &co), "bad locators rejected");
    return failures ? 1 : 0;
}
