// Solar trend port; mirrors src/widgets/solartrend/tests (the oracle) on the same fixtures
// (trimmed real NOAA SWPC responses captured 2026-10-13).
#include "../hcpcore.h"
#include "../trend.hpp"
#include "test_util.hpp"
#include <string>
#include <vector>

using namespace hcptest;

int main() {
    using namespace hcp;
    const auto kp = fixture("noaa-planetary-k-index-forecast.json");
    const auto flux = fixture("f107_cm_flux.json");
    const auto ssn = fixture("observed-solar-cycle-indices.json");
    check(!kp.empty() && !flux.empty() && !ssn.empty(), "fixtures present");

    // Kp bars: window starts 36 h before the newest observation, forecast kept.
    const auto bars = trend::kp_bars(kp).value_or(std::vector<trend::KpBar>{});
    check(!bars.empty(), "kp bars parsed");
    bool sorted = true, seen_predicted = false, predicted_tail = true;
    for (size_t i = 1; i < bars.size(); ++i) sorted = sorted && bars[i - 1].time_ms < bars[i].time_ms;
    for (const auto& b : bars) {
        seen_predicted = seen_predicted || b.predicted;
        if (seen_predicted && !b.predicted) predicted_tail = false;
    }
    check(sorted, "kp bars chronological");
    check(seen_predicted && predicted_tail, "forecast bars come last");
    int64_t newest_real = 0;
    for (const auto& b : bars)
        if (!b.predicted) newest_real = b.time_ms;
    check(!bars.empty() && bars.front().time_ms >= newest_real - 36 * 3600000LL && bars.front().time_ms <= newest_real,
          "36 h of history");
    check(bars.size() <= 32, "bounded bar count");
    const auto lone = trend::kp_bars(R"([{"time_tag":"2026-10-16T00:00:00","kp":2,"observed":"predicted"}])");
    check(lone && lone->empty(), "forecast alone is not shown");
    check(trend::kp_level(3.67) == 0 && trend::kp_level(4) == 1 && trend::kp_level(5) == 2, "kp levels");

    // Flux: one Noon reading per UTC day, at most 30 days.
    const auto days = trend::flux_daily(flux).value_or(std::vector<trend::FluxDay>{});
    check(days.size() == 30, "30 days of flux (fixture has 41)");
    bool days_sorted = true;
    for (size_t i = 1; i < days.size(); ++i) days_sorted = days_sorted && days[i - 1].day_ms < days[i].day_ms;
    check(days_sorted, "flux oldest first, one per day");
    check(!days.empty() && days.back().day_ms == 1791763200000LL && days.back().flux == 123.0,
          "newest Noon reading is Oct 12, 123 sfu");
    check(trend::flux_daily(flux, 5).value_or(std::vector<trend::FluxDay>{}).size() == 5, "limit honoured");

    const auto s = trend::latest_sunspots(ssn);
    check(s && s->month == "2026-09" && s->number == 68.73, "latest NOAA monthly SSN");

    for (const char* bad : {"", "{}", "[", "null"}) {
        check(!trend::kp_bars(bad) && !trend::flux_daily(bad) && !trend::latest_sunspots(bad), "malformed body rejected");
    }
    const auto ints = trend::kp_bars("[1,2]");
    check(ints && ints->empty() && !trend::latest_sunspots("[1,2]"), "array of junk yields nothing");
    const auto bad_flux = trend::flux_daily(
        R"([{"time_tag":"2026-10-12T20:00:00","flux":-1,"reporting_schedule":"Noon"},)"
        R"({"time_tag":"2026-10-12T20:00:00","flux":"9","reporting_schedule":"Noon"},)"
        R"({"time_tag":"2026-10-12T17:00:00","flux":99,"reporting_schedule":"Morning"}])");
    check(bad_flux && bad_flux->empty(), "bad flux values and non-Noon rows rejected");
    check(!trend::latest_sunspots(R"([{"time-tag":"2026-09","observed_swpc_ssn":-1.0}])"), "-1 missing SSN rejected");
    const auto high = trend::kp_bars(R"([{"time_tag":"2026-10-12T00:00:00","kp":12,"observed":"observed"}])");
    check(high && high->empty(), "Kp above 9 rejected");

    // C ABI used by Swift.
    std::vector<HcpKpBar> kb(64);
    const int nk = hcp_trend_kp_bars(kp.data(), kp.size(), kb.data(), kb.size());
    check(nk == static_cast<int>(bars.size()) && kb[0].time_ms == bars.front().time_ms, "C ABI kp bars");
    std::vector<HcpFluxDay> fd(64);
    check(hcp_trend_flux_daily(flux.data(), flux.size(), 30, fd.data(), fd.size()) == 30 && fd[29].flux == 123.0,
          "C ABI flux");
    char month[8];
    double number = 0;
    check(hcp_trend_sunspots(ssn.data(), ssn.size(), month, sizeof month, &number) == 1 && std::string(month) == "2026-09" &&
              number == 68.73,
          "C ABI sunspots");
    check(hcp_trend_kp_bars("{}", 2, kb.data(), kb.size()) == -1 && hcp_trend_flux_daily(nullptr, 0, 30, fd.data(), 1) == -1 &&
              hcp_trend_sunspots("{}", 2, month, sizeof month, &number) == 0,
          "C ABI rejects non-product");
    return finish("trend");
}
