#pragma once
// Qt-free port of the Passes pane (src/widgets/satellites/passes_model.cpp, the oracle): the next
// 48 h of passes at DE of every satellite with fresh elements, soonest first, with the All, Visible
// and Alerts filters. The Amateur filter needs SatNOGS transmitters and the Starred filter a tap
// target; neither exists on the TV (its element set is already CelesTrak's amateur group).
#include "satalert.hpp"
#include "domain/astro_satellite.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace hcp::passes {

enum class Filter { All, Visible, Alerts };
enum class State { Live, Loading, Stale };

constexpr int kMaxTleAgeDays = 30; // satellites_model.hpp kMaxTleAgeDays
constexpr int kMaxPasses = 24;     // passes_model.cpp kMaxPasses

struct Row {
    std::string name;            // short name ("SO-50")
    int64_t aos = 0, los = 0;    // unix seconds
    std::string day;             // "Today", "Tomorrow" or "Sun" (UTC days)
    std::string utc;             // "14:05Z"
    int max_el_deg = 0;
    std::string from, to;        // 16-point compass at AOS and LOS
    std::string duration;        // "12 min"
    std::string mag;             // "-1.8" for known large objects, else empty
    bool visible = false;        // sunlit in a dark sky
    bool live = false;           // in progress
    bool alert = false;          // selected for pass alerts
};

struct Result {
    State state = State::Loading;
    std::vector<Row> rows;
    std::string empty_reason;
};

// "Today", "Tomorrow" or the C-locale weekday ("Sun") of a UTC unix time relative to now.
std::string day_label(int64_t unix_time, int64_t now);

// Sets whose epoch is within kMaxTleAgeDays of unix_time (fresh_tles in satellites_model.cpp).
std::vector<hamclock::Tle> fresh_tles(const std::vector<hamclock::Tle>& tles, double unix_time);

// PassesModel::recompute for a configured station. `alerts` and `catalog` decide the Alerts
// filter and the alert mark (pass alerts off: no marks, the Alerts filter says so).
Result schedule(const std::vector<hamclock::Tle>& tles, const hamclock::Observer& obs, int64_t now, Filter filter,
                const satalert::Prefs& alerts, const satalert::Catalog& catalog);

} // namespace hcp::passes
