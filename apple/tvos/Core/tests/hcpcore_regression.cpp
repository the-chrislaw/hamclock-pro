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
    // Sun/Moon events at JN58td (48.1N 11.6E) around the 2026-03-20 equinox: ~12 h days.
    int64_t rise = 0, set = 0;
    const int64_t t0 = 1774017960LL;
    check(hcp_next_sun_event(t0, 0, 48.1458, 11.625, &rise) && rise > t0 && rise - t0 <= 86400, "next sunrise within a day");
    check(hcp_next_sun_event(t0, 1, 48.1458, 11.625, &set) && set > t0 && set - t0 <= 86400, "next sunset within a day");
    check(!hcp_next_sun_event(t0, 2, 48.1458, 11.625, &set), "bad kind rejected");
    check(!hcp_next_sun_event(t0, 0, 95.0, 0, &rise), "bad latitude rejected");
    check(!hcp_next_sun_event(1782000000LL, 0, 89.0, 0, &rise), "polar day has no sunrise");
    int64_t mr = 0;
    check(hcp_next_moon_event(t0, 0, 48.1458, 11.625, &mr) && mr > t0 && mr - t0 <= 3 * 86400, "next moonrise");
    double ill = -1;
    check(hcp_moon_phase(t0, buf, sizeof buf, &ill) && ill >= 0 && ill <= 1, "moon phase");
    check(!hcp_moon_phase(t0, buf, sizeof buf, nullptr), "null illum rejected");
    // Satellites: ISS element set (epoch 2008-264, Vallado/Celestrak sample).
    const char tle[] =
        "ISS (ZARYA)\n"
        "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927\n"
        "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537\n";
    check(hcp_tle_count(tle, sizeof tle - 1) == 1, "one TLE parsed");
    check(hcp_tle_count("garbage", 7) == 0 && hcp_tle_count(nullptr, 0) == 0, "bad TLE text rejected");
    check(hcp_tle_name(tle, sizeof tle - 1, 0, buf, sizeof buf) && std::string(buf) == "ISS (ZARYA)", "TLE name");
    HcpSatState ss{};
    const int64_t te = 1221931538LL; // epoch + 5 h
    check(hcp_sat_state(tle, sizeof tle - 1, 0, te, 48.1, 11.6, &ss), "sat state");
    check(ss.alt_km > 330 && ss.alt_km < 400, "ISS altitude plausible");
    check(std::abs(ss.lat_deg) <= 51.7 && ss.el_deg >= -90 && ss.el_deg <= 90 && ss.range_km > 300, "ISS geometry plausible");
    check(std::abs(ss.epoch_s - 1221913538LL) < 5, "TLE epoch exposed");
    check(!hcp_sat_state(tle, sizeof tle - 1, 1, te, 48.1, 11.6, &ss), "index out of range");
    check(!hcp_sat_state(tle, sizeof tle - 1, 0, te, 95.0, 11.6, &ss), "bad latitude rejected");
    // Pass search (iPhone widgets): the ISS passes over 48.1N several times a day.
    HcpPass pp[8];
    const int np = hcp_sat_passes(tle, sizeof tle - 1, 0, te, 86400, 48.1, 11.6, 0.0, pp, 8);
    check(np >= 2 && np <= 8, "ISS passes found within a day");
    bool ordered = np > 0;
    for (int i = 0; i < np; ++i) {
        ordered = ordered && pp[i].aos_s >= te && pp[i].aos_s <= pp[i].tca_s && pp[i].tca_s <= pp[i].los_s &&
                  pp[i].los_s - pp[i].aos_s < 20 * 60 && pp[i].max_el_deg >= 0 && pp[i].max_el_deg <= 90 &&
                  pp[i].aos_az_deg >= 0 && pp[i].aos_az_deg < 360 && pp[i].los_az_deg >= 0 && pp[i].los_az_deg < 360;
        if (i > 0) ordered = ordered && pp[i].aos_s > pp[i - 1].los_s;
    }
    check(ordered, "passes ordered, short and with valid azimuths");
    // During a pass the satellite is already up: the first pass then starts at `from`.
    if (np > 0) {
        const int64_t mid = pp[0].tca_s;
        HcpPass up[1];
        check(hcp_sat_passes(tle, sizeof tle - 1, 0, mid, 3600, 48.1, 11.6, 0.0, up, 1) == 1 && up[0].aos_s == mid &&
                  up[0].los_s == pp[0].los_s, "pass in progress starts now and keeps its LOS");
    }
    HcpPass high[8];
    const int nh = hcp_sat_passes(tle, sizeof tle - 1, 0, te, 86400, 48.1, 11.6, 30.0, high, 8);
    bool high_ok = nh >= 0 && nh <= np;
    for (int i = 0; i < nh; ++i) high_ok = high_ok && high[i].max_el_deg >= 30.0;
    check(high_ok, "minimum elevation filters low passes");
    check(hcp_sat_passes(tle, sizeof tle - 1, 0, te, 86400, 48.1, 11.6, 0.0, pp, 1) == 1, "cap respected");
    check(hcp_sat_passes(tle, sizeof tle - 1, 1, te, 86400, 48.1, 11.6, 0.0, pp, 8) == -1, "pass index out of range");
    check(hcp_sat_passes(tle, sizeof tle - 1, 0, te, 86400, 95.0, 11.6, 0.0, pp, 8) == -1, "pass bad latitude");
    check(hcp_sat_passes(nullptr, 0, 0, te, 86400, 48.1, 11.6, 0.0, pp, 8) == -1 &&
              hcp_sat_passes(tle, sizeof tle - 1, 0, te, 86400, 48.1, 11.6, 0.0, nullptr, 8) == -1 &&
              hcp_sat_passes(tle, sizeof tle - 1, 0, te, 0, 48.1, 11.6, 0.0, pp, 8) == -1, "pass null/empty input");
    // Short names follow satellite_short_name (src/widgets/satellites): the Qt app's tracked= form.
    const char two[] =
        "ISS (ZARYA)\n"
        "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927\n"
        "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537\n"
        "SAUDISAT 1C (SO-50)\n"
        "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927\n"
        "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537\n"
        "FUNCUBE-1 & AO-73\n"
        "1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927\n"
        "2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537\n";
    char sn[32];
    check(hcp_tle_short_name(two, sizeof two - 1, 0, sn, sizeof sn) && std::string(sn) == "ISS", "ISS short name");
    check(hcp_tle_short_name(two, sizeof two - 1, 1, sn, sizeof sn) && std::string(sn) == "SO-50", "bracketed short name");
    check(hcp_tle_short_name(two, sizeof two - 1, 2, sn, sizeof sn) && std::string(sn) == "FUNCUBE-1", "ampersand short name");
    check(!hcp_tle_short_name(two, sizeof two - 1, 3, sn, sizeof sn), "short name index range");
    check(hcp_tle_find_short_name(two, sizeof two - 1, "so-50") == 1 && hcp_tle_find_short_name(two, sizeof two - 1, "ISS") == 0,
          "find by short name, case-insensitive");
    check(hcp_tle_find_short_name(two, sizeof two - 1, "AO-7") == -1 && hcp_tle_find_short_name(two, sizeof two - 1, "") == -1 &&
              hcp_tle_find_short_name(nullptr, 0, "ISS") == -1, "unknown or empty name not found");
    // Coastline table.
    const int parts = hcp_coast_part_count();
    size_t npts = 0;
    const float* pts = hcp_coast_points(&npts);
    check(parts > 100 && pts && npts > 1000, "coastline table exposed");
    uint32_t cs = 0, cc = 0;
    check(hcp_coast_part(0, &cs, &cc) && cc >= 2 && cs + cc <= npts, "first coast part in range");
    check(hcp_coast_part(parts - 1, &cs, &cc) && cs + cc <= npts, "last coast part in range");
    check(!hcp_coast_part(parts, &cs, &cc) && !hcp_coast_part(-1, &cs, &cc), "coast index range checked");
    bool inRange = true;
    for (size_t i = 0; i < npts; ++i)
        if (pts[2 * i] < -180.f || pts[2 * i] > 180.f || pts[2 * i + 1] < -90.f || pts[2 * i + 1] > 90.f) inRange = false;
    check(inRange, "coastline coordinates are valid lon/lat");
    // HF outlook: same expectations as src/widgets/bands/tests.
    HcpBand hb[8];
    auto find = [&](int n, const char* name) -> const HcpBand* {
        for (int i = 0; i < n; ++i) if (std::string(hb[i].band) == name) return &hb[i];
        return nullptr;
    };
    int nb = hcp_hf_outlook(123, 3, 15, hb, 8);
    check(nb == 8 && find(nb, "20m") && find(nb, "20m")->day == 2, "20m good by day at SFI 123");
    check(find(nb, "10m")->day == 0 && find(nb, "10m")->night == 0, "10m closed at SFI 123");
    check(find(nb, "80m")->day == 0, "80m poor by day (D layer)");
    nb = hcp_hf_outlook(200, 3, 0, hb, 8);
    check(find(nb, "10m")->day == 2 && find(nb, "10m")->night == 0, "10m open by day only at SFI 200");
    int calm20 = find(nb, "20m")->day;
    nb = hcp_hf_outlook(200, 5, 0, hb, 8);
    check(find(nb, "20m")->day == calm20 - 1, "Kp 5 lowers a band one step");
    nb = hcp_hf_outlook(200, 7, 0, hb, 8);
    check(find(nb, "20m")->day == 0, "Kp 7 lowers two steps");
    check(hcp_hf_outlook(123, 3, 15, hb, 3) == 3 && !hcp_hf_outlook(1, 1, 1, nullptr, 8), "cap and null respected");
    return failures ? 1 : 0;
}
