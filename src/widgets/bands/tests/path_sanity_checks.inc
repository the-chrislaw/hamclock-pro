// Shared by path_model_regression.cpp (Qt) and apple/tvos/Core/tests/pathrel_regression.cpp (the
// port), so both models answer to the same sanity cases. Needs check(), band() and the model's
// PathInput, path_outlook and muf_3000_mhz in scope.
//
// References for the expected values: ITU-R P.533-14 Table 1a and sec 3.5.1.2 (control points of a
// long path), P.1240-2 sec 3.2 and Table 1 (operational MUF), and the CCIR maps of P.1239 for the
// foF2 and M(3000)F2 numbers below. Test station W1AW (FN31pr), DX Tokyo (PM95uq).
namespace path_sanity {

constexpr std::int64_t kNov1 = 1793491200, kJun15 = 1781481600; // 2026-11-01 and 2026-06-15, 00:00 UTC
const char* const kHighBands[] = {"20m", "17m", "15m", "12m", "10m"};

PathInput path_to(double lat, double lon) { return PathInput{41.73, -72.71, lat, lon}; }

double reliability_at(const PathInput& p, std::int64_t day, int hour, const char* name, double sfi, double kp) {
    return band(path_outlook(p, UtcSeconds{day + 3600LL * hour}, sfi, kp), name).reliability;
}
// Whole hours in [from, to) when the band reaches the threshold.
int open_hours(const PathInput& p, std::int64_t day, const char* name, double sfi, double kp, double threshold = 0.25,
               int from = 0, int to = 24) {
    int hours = 0;
    for (int h = from; h < to; ++h) hours += reliability_at(p, day, h, name, sfi, kp) >= threshold ? 1 : 0;
    return hours;
}

void run_sanity_cases() {
    const PathInput tokyo = path_to(35.69, 139.71), london = path_to(51.5, -0.1);
    const PathInput short_path = path_to(41.0, -84.5); // about 1000 km
    const PathInput low_latitude = path_to(0.02, 0.04); // JJ00, stays well away from the auroral zone
    const double sfi = 92, quiet = 1.5;

    // Northeast US to Europe: 20m opens for part of the day, 40m and 80m at night.
    check(open_hours(london, kNov1, "20m", sfi, quiet) >= 3, "W1AW-London 20m opens by day at SFI 92 (Nov)");
    check(open_hours(london, kJun15, "20m", sfi, quiet) >= 4, "W1AW-London 20m opens by day at SFI 92 (Jun)");
    check(open_hours(london, kNov1, "40m", sfi, quiet, 0.5, 0, 8) >= 4, "W1AW-London 40m open at night");
    check(open_hours(london, kNov1, "80m", sfi, quiet, 0.5, 0, 8) >= 4, "W1AW-London 80m open at night");

    // Northeast US to Japan: 20m or 17m opens for some hours, 40m around shared darkness (08-11Z).
    check(open_hours(tokyo, kNov1, "20m", sfi, quiet) + open_hours(tokyo, kNov1, "17m", sfi, quiet) >= 2,
          "W1AW-Tokyo 20m or 17m opens at SFI 92");
    check(open_hours(tokyo, kNov1, "40m", sfi, quiet, 0.5, 8, 12) >= 3, "W1AW-Tokyo 40m open in shared darkness");
    check(open_hours(tokyo, kNov1, "20m", 200, quiet) >= 6, "W1AW-Tokyo 20m well open at SFI 200");

    // 10m closed or nearly so at SFI 92; a high flux opens it.
    check(open_hours(tokyo, kNov1, "10m", sfi, quiet) <= 1, "W1AW-Tokyo 10m nearly closed at SFI 92");
    check(open_hours(london, kNov1, "10m", sfi, quiet) <= 4, "W1AW-London 10m at most marginal at SFI 92");
    check(open_hours(london, kNov1, "10m", 200, quiet) >= 4, "W1AW-London 10m opens at SFI 200");

    // A higher flux opens higher bands, monotonically.
    bool monotonic = true;
    for (const char* name : kHighBands) {
        for (const auto& p : {tokyo, london}) {
            int previous = -1;
            for (double flux : {70.0, 92.0, 120.0, 150.0, 200.0, 250.0}) {
                const int hours = open_hours(p, kNov1, name, flux, quiet);
                monotonic = monotonic && hours >= previous;
                previous = hours;
            }
        }
    }
    check(monotonic, "open hours never fall as SFI rises, 20m to 10m");

    // Daytime short path: 40m and 20m open, 80m absorbed (17Z is about local noon).
    const auto noon = path_outlook(short_path, UtcSeconds{kNov1 + 17 * 3600}, sfi, quiet);
    const auto midnight = path_outlook(short_path, UtcSeconds{kNov1 + 5 * 3600}, sfi, quiet);
    check(band(noon, "40m").reliability >= 0.5, "1000 km 40m open by day");
    check(band(noon, "20m").reliability >= 0.25, "1000 km 20m open by day");
    check(band(noon, "80m").snr_db <= band(noon, "40m").snr_db - 3, "1000 km 80m weaker than 40m by day");
    check(band(noon, "80m").snr_db <= band(midnight, "80m").snr_db - 10, "1000 km 80m loses at least 10 dB by day");
    check(band(midnight, "80m").reliability >= 0.5, "1000 km 80m open at night");

    // A high Kp reduces reliability, more on a high-latitude path.
    for (const auto& p : {tokyo, london}) {
        int previous = 1000;
        bool falling = true;
        for (double kp : {0.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}) {
            const int hours = open_hours(p, kNov1, "40m", sfi, kp, 0.1) + open_hours(p, kNov1, "20m", sfi, kp, 0.1);
            falling = falling && hours <= previous;
            previous = hours;
        }
        check(falling, "open hours never rise with Kp");
    }
    const double london_kp4 = reliability_at(london, kNov1, 15, "20m", sfi, 4) / reliability_at(london, kNov1, 15, "20m", sfi, 1);
    const double low_lat_kp4 = reliability_at(low_latitude, kNov1, 15, "20m", sfi, 4) / reliability_at(low_latitude, kNov1, 15, "20m", sfi, 1);
    check(london_kp4 < low_lat_kp4, "Kp 4 hurts the high-latitude path more");

    // No overcorrection: not everything is open, and at every hour some band is closed.
    int band_hours = 0, closed_hours = 0, usable_hours = 0, named_hours = 0;
    for (int h = 0; h < 24; ++h) {
        const auto column = path_outlook(tokyo, UtcSeconds{kNov1 + 3600LL * h}, sfi, quiet);
        double best = 0, worst = 1;
        for (const auto& b : column) {
            band_hours += b.reliability >= 0.5 ? 1 : 0;
            best = std::max(best, b.reliability);
            worst = std::min(worst, b.reliability);
        }
        closed_hours += worst < 0.1 ? 1 : 0;
        usable_hours += best >= 0.25 ? 1 : 0;
        named_hours += best >= 0.05 ? 1 : 0; // below 5% the pane says "HF closed now"
    }
    check(band_hours <= 96, "W1AW-Tokyo: at most half the band-hours above 50%");
    check(closed_hours == 24, "W1AW-Tokyo: some band closed at every hour");
    check(usable_hours >= 20, "W1AW-Tokyo: some band above 25% at nearly every hour");
    check(named_hours == 24, "W1AW-Tokyo: never 'HF closed now' at SFI 92, quiet");

    // Path MUF against P.533: the lower F2(dmax)MUF of the control points d0/2 from each end, from
    // the CCIR maps at SFI 92 in November, MHz, with the P.1240 operational ratio on top. A dark
    // mid-path must not set it.
    struct MufCase { int hour; double basic_mhz; };
    for (const MufCase c : {MufCase{0, 14.1}, MufCase{4, 8.7}, MufCase{10, 7.6}, MufCase{16, 9.9}, MufCase{22, 19.9}}) {
        const double muf = band(path_outlook(tokyo, UtcSeconds{kNov1 + 3600LL * c.hour}, sfi, quiet), "20m").muf_mhz;
        check(muf >= 0.85 * c.basic_mhz && muf <= 1.75 * c.basic_mhz, "W1AW-Tokyo MUF tracks P.533 at this hour");
    }

    // MUF(3000) = foF2 * M(3000)F2 against the CCIR maps at SFI 92 in November. The tolerance is
    // the fit's own: foF2 is about 13% rms at midday and 20% before dawn, worst at some longitudes.
    struct MapCase { double lat, lon; int hour; double mhz; };
    for (const MapCase c : {MapCase{37.9, -75.5, 17, 26.7},  // Wallops local noon
                            MapCase{37.9, -75.5, 9, 9.6},    // Wallops before dawn
                            MapCase{35.7, 139.5, 3, 29.4},   // Kokubunji local noon
                            MapCase{51.6, -1.3, 12, 25.9},   // Chilton local noon
                            MapCase{51.6, -1.3, 4, 6.1}}) {  // Chilton before dawn
        const double muf = muf_3000_mhz(sfi, c.lat, c.lon, UtcSeconds{kNov1 + 3600LL * c.hour});
        check(muf >= 0.6 * c.mhz && muf <= 1.4 * c.mhz, "MUF(3000) within 40% of the CCIR map");
    }
}

} // namespace path_sanity
