// Passes pane port; mirrors src/widgets/satellites/tests/passes_regression.cpp (the oracle): 48 h
// schedule rows from the CelesTrak amateur fixture, the All / Visible / Alerts filters, honest
// loading and stale states, plus the whole set through the C ABI.
#include "../hcpcore.h"
#include "../passes.hpp"
#include "../satalert.hpp"
#include "../satpass.hpp"
#include "test_util.hpp"
#include <string>
#include <vector>

using namespace hcptest;
using namespace hcp::passes;

int main() {
    const std::string text = fixture("celestrak-amateur.tle");
    auto tles = hamclock::parse_tle_set(text);
    check(tles.size() >= 5, "several element sets");
    if (tles.empty()) return finish("passes");
    const std::vector<hamclock::Tle> all_sets = tles;
    // The Qt test schedules five birds; the same five keep this quick.
    tles.resize(5);
    const int64_t t0 = static_cast<int64_t>(hamclock::SatellitePropagator(tles.front()).epoch_unix()) + 3600;
    const hamclock::Observer obs{45.52, -122.68, 0.0}; // the Qt test's station
    const hcp::satalert::Catalog no_catalog;
    const hcp::satalert::Prefs alerts_off;

    check(day_label(t0, t0) == "Today" && day_label(t0 + 86400, t0) == "Tomorrow" &&
              day_label(1792800000, 1792800000 - 3 * 86400).size() == 3,
          "day labels");
    check(day_label(1793275200, 1793275200 - 3 * 86400) == "Thu", "weekday of 2026-10-29 is Thu");

    const Result none = schedule({}, obs, t0, Filter::All, alerts_off, no_catalog);
    check(none.state == State::Loading && none.rows.empty() && !none.empty_reason.empty(), "no elements yet: loading");

    const Result live = schedule(tles, obs, t0, Filter::All, alerts_off, no_catalog);
    const auto& rows = live.rows;
    check(live.state == State::Live && rows.size() >= 3 && rows.size() <= 24, "live with a handful of passes");
    bool sorted = true, sane = true;
    int64_t prev = 0;
    for (const auto& r : rows) {
        sorted = sorted && r.aos >= prev;
        prev = r.aos;
        sane = sane && r.max_el_deg >= 10 && r.max_el_deg <= 90 && !r.name.empty() && r.utc.size() == 6 &&
               r.utc.back() == 'Z' && !r.from.empty() && !r.to.empty() && r.duration.size() > 4 &&
               r.duration.substr(r.duration.size() - 4) == " min" && r.los > t0 && !r.alert;
    }
    check(sane && sorted, "rows well-formed");

    const Result vis = schedule(tles, obs, t0, Filter::Visible, alerts_off, no_catalog);
    for (const auto& r : vis.rows) check(r.visible, "visible filter keeps visible passes only");
    check(vis.rows.size() <= rows.size(), "visible filter never adds passes");
    if (vis.rows.empty()) check(vis.empty_reason.find("visible") != std::string::npos, "visible empty reason");

    const Result off = schedule(tles, obs, t0, Filter::Alerts, alerts_off, no_catalog);
    check(off.rows.empty() && off.empty_reason == "Turn on satellite alerts in Settings", "alerts filter with alerts off");

    // Alerts on, only the first bird picked: the filter keeps it, All marks it.
    hcp::satalert::Prefs picked;
    picked.enabled = true;
    picked.all_workable = false;
    const int first_norad = hcp::satalert::norad_id(tles.front());
    picked.picked = {first_norad};
    const std::string first = hcp::sats::short_name(tles.front().name);
    const Result al = schedule(tles, obs, t0, Filter::Alerts, picked, no_catalog);
    bool only = !al.rows.empty();
    for (const auto& r : al.rows) only = only && r.name == first && r.alert;
    check(only, "alerts filter keeps only the selected satellite");
    bool marked = false, unmarked = true;
    for (const auto& r : schedule(tles, obs, t0, Filter::All, picked, no_catalog).rows) {
        if (r.name == first) marked = marked || r.alert;
        else unmarked = unmarked && !r.alert;
    }
    check(marked && unmarked, "All marks alert passes only");

    // A pass in progress is listed as live.
    bool found_live = false;
    for (const auto& r : rows)
        if (!found_live && r.aos > t0) {
            const Result mid = schedule(tles, obs, (r.aos + r.los) / 2, Filter::All, alerts_off, no_catalog);
            found_live = !mid.rows.empty() && mid.rows.front().live && mid.rows.front().day == day_label(r.aos, r.aos);
        }
    check(found_live, "in-progress pass is first and live");

    // Elements far older than the cut-off are dropped, not scheduled.
    const Result old = schedule(tles, obs, t0 + 60 * 86400, Filter::All, alerts_off, no_catalog);
    check(old.state == State::Stale && old.rows.empty(), "old elements -> stale, no rows");

    // The whole amateur set through the C ABI, as the TV calls it.
    std::vector<HcpPassRow> out(64);
    int state = -1;
    char reason[96];
    const int n = hcp_passes_schedule(text.data(), text.size(), nullptr, 0, nullptr, 41.71, -72.73, t0, 0, out.data(),
                                      out.size(), &state, reason, sizeof reason);
    bool abi_sorted = n > 0;
    for (int i = 1; i < n; ++i) abi_sorted = abi_sorted && out[static_cast<size_t>(i - 1)].aos_s <= out[static_cast<size_t>(i)].aos_s;
    check(n == kMaxPasses && state == 0 && abi_sorted && out[0].name[0] != 0 && out[0].utc[5] == 'Z',
          "C ABI: 24 sorted rows over the full set");
    check(all_sets.size() >= 90, "fixture holds the amateur set");
    check(hcp_passes_schedule(text.data(), text.size(), nullptr, 0, nullptr, 91, 0, t0, 0, out.data(), out.size(), &state,
                              reason, sizeof reason) == -1 &&
              hcp_passes_schedule(text.data(), text.size(), nullptr, 0, nullptr, 0, 0, t0, 7, out.data(), out.size(),
                                  &state, reason, sizeof reason) == -1,
          "C ABI rejects bad observer and filter");
    const int stale = hcp_passes_schedule(text.data(), text.size(), nullptr, 0, nullptr, 41.71, -72.73, t0 + 90 * 86400, 0,
                                          out.data(), out.size(), &state, reason, sizeof reason);
    check(stale == 0 && state == 2 && std::string(reason) == "Satellite elements are too old", "C ABI stale state");
    return finish("passes");
}
