#pragma once
// Qt-free port of src/widgets/launches/launches_parse (Launch Library 2 upcoming list,
// ll.thespacedevs.com/2.2.0/launch/upcoming/?mode=list). Same rules as the Qt parser.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::launches {

struct Launch {
    std::string name, provider, pad, location;
    std::string status;        // abbreviation: Go, TBC, TBD, Success, Hold ...
    std::string mission, mission_type;
    int64_t net_ms = 0;        // no-earlier-than time, UTC
    bool precise = false;      // time accurate to the hour or better
};

// Soonest first. Rows with no name or an unreadable time are skipped.
// nullopt when the body has no "results" array.
std::optional<std::vector<Launch>> parse(std::string_view body);

// Amateur-interest hint from the name/mission text only; empty when nothing matches.
std::string ham_hint(const Launch& launch);

struct PadPosition { double lat, lon; };
// Approximate site position from the location text (built-in spaceport table); nullopt when unknown.
std::optional<PadPosition> pad_position(const Launch& launch);

// "T-2d 3h", "T-5h 12m", "T-12m", "T+0m" once past.
std::string countdown_text(int64_t net_ms, int64_t now_ms);

} // namespace hcp::launches
