#pragma once
// Qt-free port of src/widgets/solaralerts/solar_alerts_parse (NOAA SWPC products/alerts.json:
// alerts, watches and warnings). Same rules as the Qt parser, which stays the behavioural oracle.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::alerts {

struct Alert {
    std::string code;       // SWPC product id, e.g. "K05A"
    std::string kind;       // Alert, Warning, Extended warning, Watch, Summary, Cancel
    std::string headline;   // e.g. "Geomagnetic K-index of 5"
    std::string scale;      // NOAA scale such as "G1", "S2", "R3"; empty if none stated
    int64_t issued_ms = 0;  // UTC
    int64_t until_ms = 0;   // UTC end of validity; 0 when the message has none
    bool cancelled = false; // a cancellation notice, not an event
};

// All parseable messages, newest first. Messages without a headline, issue time or product id are
// skipped. nullopt when the body is not a JSON array (the Qt parser then returns an empty list).
std::optional<std::vector<Alert>> parse(std::string_view body);

// What is in force at now_ms: newest message per product id; cancellations dropped; warnings only
// until their end time; alerts and summaries for 24 h; watches (no end time) for 48 h.
std::vector<Alert> active(const std::vector<Alert>& all, int64_t now_ms);

// 0 info, 1 minor/moderate (scale 1-2 or a watch/warning), 2 strong+ (scale >= 3).
int level(const Alert& alert);

// "2026 Oct 13 1431" (after " UTC" is removed) -> epoch ms; 0 when it does not parse.
int64_t parse_swpc_time(std::string_view text);

} // namespace hcp::alerts
