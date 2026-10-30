// Path reliability model: physical sanity of the in-house estimate (offline).
#include "../path_model.hpp"
#include <cmath>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
using namespace hamclock;
using namespace hamclock::bands;
const PathBand& band(const std::vector<PathBand>& v, const char* name) {
    for (const auto& b : v) if (b.band == name) return b;
    static const PathBand none{};
    return none;
}
// 2026-10-13 12:00 UTC and 00:00 UTC.
constexpr std::int64_t kNoonUtc = 1760356800, kMidnightUtc = 1760313600;
} // namespace

int main() {
    // London -> New York, ~5570 km, 2 hops.
    PathInput lon_ny{51.5, -0.1, 40.7, -74.0};
    const double d = path_distance_km(lon_ny);
    check(d > 5450 && d < 5700, "London-New York distance");

    const auto noon = path_outlook(lon_ny, UtcSeconds{1760367600}, 150, 2); // 15:00 UTC, daylight at both ends
    check(noon.size() == 8, "eight bands");
    check(band(noon, "20m").hops == 2, "two hops");
    check(band(noon, "20m").takeoff_deg > 2 && band(noon, "20m").takeoff_deg < 15, "takeoff angle plausible");
    check(band(noon, "20m").reliability > 0.6, "20m open by day at SFI 150");
    check(band(noon, "10m").reliability > band(noon, "80m").reliability, "80m is absorbed by day");
    check(band(noon, "80m").reliability < 0.2, "80m poor by day");

    const auto night = path_outlook(lon_ny, UtcSeconds{kMidnightUtc + 3 * 3600}, 150, 2);
    check(band(night, "80m").reliability > band(noon, "80m").reliability, "80m better at night");
    check(band(night, "10m").reliability < 0.2, "10m closed at night");

    // Higher flux raises the MUF; a storm lowers reliability.
    const auto low = path_outlook(lon_ny, UtcSeconds{1760367600}, 70, 2);
    const auto high = path_outlook(lon_ny, UtcSeconds{1760367600}, 220, 2);
    check(band(high, "10m").muf_mhz > band(low, "10m").muf_mhz, "MUF rises with SFI");
    check(band(high, "10m").reliability > band(low, "10m").reliability, "10m opens with SFI");
    const auto storm = path_outlook(lon_ny, UtcSeconds{1760367600}, 150, 7);
    check(band(storm, "20m").reliability < 0.5 * band(noon, "20m").reliability, "Kp 7 cuts reliability");

    // Power, gain and mode matter on a marginal band.
    PathInput qrp = lon_ny;
    qrp.power_w = 1;
    check(band(path_outlook(qrp, UtcSeconds{kNoonUtc + 6 * 3600}, 150, 2), "15m").snr_db <
              band(noon, "15m").snr_db - 15, "100x less power is 20 dB less SNR");
    PathInput ft8 = qrp;
    ft8.mode = PathMode::Digital;
    check(band(path_outlook(ft8, UtcSeconds{kNoonUtc + 6 * 3600}, 150, 2), "20m").reliability >=
              band(path_outlook(qrp, UtcSeconds{kNoonUtc + 6 * 3600}, 150, 2), "20m").reliability, "digital needs less SNR");

    // Short path: single hop, local NVIS works on 40m by day and 80m at night.
    PathInput local{51.5, -0.1, 53.5, -2.2};
    check(band(path_outlook(local, UtcSeconds{1760367600}, 120, 1), "40m").hops == 1, "short path one hop");

    // Long path: the other way round the globe, so ~34400 km and many more hops, never better than short.
    PathInput lp = lon_ny;
    lp.long_path = true;
    const double ld = path_distance_km(lp);
    check(ld > 34300 && ld < 34500 && std::abs(ld + d - 40030) < 100, "long path is the rest of the circumference");
    const auto lp_noon = path_outlook(lp, UtcSeconds{1760367600}, 150, 2);
    check(band(lp_noon, "20m").hops >= 11, "long path needs many hops");
    check(band(lp_noon, "20m").reliability < band(noon, "20m").reliability, "long path is worse than short");
    bool lp_finite = true;
    for (const auto& b : lp_noon) lp_finite = lp_finite && std::isfinite(b.reliability) && std::isfinite(b.muf_mhz);
    check(lp_finite, "long path stays finite");

    // Daytime W1AW (FN31pr) -> JJ00AA, ~8600 km, 3 hops, at a modest SFI 89: 20m is the
    // classic open band here, not "HF closed" (the band-conditions widget rates it Good).
    PathInput w1aw_africa{41.71, -72.73, 0.02, 0.04};
    const auto africa = path_outlook(w1aw_africa, UtcSeconds{1761753600}, 89, 1); // 2025-10-29 16:00 UTC
    check(band(africa, "20m").hops == 3, "W1AW-JJ00 three hops");
    check(band(africa, "20m").reliability > 0.4, "W1AW-JJ00 20m open by day at SFI 89");
    check(band(africa, "80m").reliability < 0.05, "W1AW-JJ00 80m absorbed by day");
    check(band(africa, "20m").snr_db > 10, "W1AW-JJ00 20m SNR above the SSB need");

    // Bad input is clamped, never NaN.
    const auto odd = path_outlook(lon_ny, UtcSeconds{kNoonUtc}, std::nan(""), std::nan(""));
    bool finite = true;
    for (const auto& b : odd) finite = finite && std::isfinite(b.reliability) && std::isfinite(b.snr_db);
    check(finite, "NaN inputs stay finite");

    // Matrix: 8 bands x 24 hours, values in [0, 1].
    const auto m = path_matrix(lon_ny, UtcSeconds{kMidnightUtc}, 24, 150, 2);
    bool ok = m.size() == 8;
    for (const auto& row : m) {
        ok = ok && row.size() == 24;
        for (double v : row) ok = ok && v >= 0 && v <= 1;
    }
    check(ok, "matrix shape and range");
    std::cout << "Path model: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
