#pragma once
// Qt-free port of src/widgets/solartrend/solar_trend_parse (NOAA SWPC 3-day Kp forecast, daily
// 10.7 cm flux history, monthly sunspot number). Same rules as the Qt parser, the behavioural oracle.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::trend {

struct KpBar {
    int64_t time_ms = 0;    // UTC start of the 3-hour interval
    double kp = 0;
    bool predicted = false; // forecast rather than observed/estimated
};

struct FluxDay {
    int64_t day_ms = 0; // UTC midnight of the day
    double flux = 0;    // sfu
};

struct Sunspots {
    std::string month; // "YYYY-MM"
    double number = 0;
};

// products/noaa-planetary-k-index-forecast.json: bars from 36 h before the newest non-predicted bar to the
// end of the forecast, oldest first; Kp outside [0, 9] skipped; empty when there is no observation.
// nullopt when the body is not a JSON array.
std::optional<std::vector<KpBar>> kp_bars(std::string_view body);

// json/f107_cm_flux.json: the Noon reading per UTC day (0 < flux <= 1000), at most `days` newest, oldest first.
std::optional<std::vector<FluxDay>> flux_daily(std::string_view body, int days = 30);

// json/solar-cycle/observed-solar-cycle-indices.json: newest month with a non-negative NOAA sunspot number.
std::optional<Sunspots> latest_sunspots(std::string_view body);

// 0 quiet, 1 active (Kp 4), 2 storm (Kp >= 5, NOAA G1+).
int kp_level(double kp);

} // namespace hcp::trend
