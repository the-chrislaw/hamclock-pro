#pragma once
// Qt-free port of src/widgets/spaceweather/swpc_parse (NOAA SWPC JSON products).
// Same rules as the Qt parser, which stays the behavioural oracle in tests.
// Pure functions over the raw body: no network, no clock. Times are UTC epoch ms.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::swpc {

struct Reading {
    double value = 0;
    int64_t time_ms = 0; // UTC observation time from the product
};

// How old a shown value may be, measured from the product's own time tag: a feed can answer on
// time with rows that have stopped moving. Same limits as src/widgets/spaceweather/swpc_parse.hpp.
// Kp: the tag is the START of a 3 h interval and the row appears after its end, so a healthy feed
// reaches 6 h just before the next row. The limit allows one missed row.
inline constexpr int64_t kKpMaxAgeMs = 9 * 3600 * 1000LL;
// SFI: one reading a day, tagged 20:00 UTC. 72 h covers a Friday reading still showing on Monday
// evening, and 12 h more lets Monday's arrive late.
inline constexpr int64_t kSolarFluxMaxAgeMs = 84 * 3600 * 1000LL;
// X-ray: a new row every minute. Matches the X-ray product's fetch limit.
inline constexpr int64_t kXrayMaxAgeMs = 30 * 60 * 1000LL;

// False for an unknown time (<= 0) and for a tag ahead of the clock.
bool is_old(int64_t time_ms, int64_t now_ms, int64_t max_age_ms);

// One index a pane depends on, as the pane sees it now.
struct Index {
    const char* label = "";
    int64_t max_age_ms = 0;
    bool has_value = false;
    bool waiting = false;   // no value and no answer yet: neither old nor missing
    bool feed_late = false; // the feed itself is past its fetch limit
    int64_t time_ms = 0;
};

struct IndexCheck {
    bool stale = false;     // something is old or missing
    int64_t oldest_ms = 0;  // time tag of the oldest old reading; 0 when none is old
    // Footer text, longest first; panes show the first that fits. Empty when not stale.
    std::string note;       // "SWPC · old Kp, SFI, no X-ray", the main app's wording
    std::string short_note; // "old Kp, SFI"
    std::string tiny_note;  // "old data"
};

// Indices in the order the footer names them.
IndexCheck check_indices(const std::vector<Index>& indices, int64_t now_ms);

// ISO-8601 "YYYY-MM-DDTHH:MM:SS[.fff][Z]" (UTC assumed) -> epoch ms.
std::optional<int64_t> parse_iso_utc_ms(std::string_view text);

std::optional<Reading> latest_kp(std::string_view body);          // Kp in [0, 9]
std::optional<Reading> latest_solar_flux(std::string_view body);  // sfu, (0, 1000]
std::optional<Reading> latest_xray_flux(std::string_view body);   // 0.1-0.8nm, W/m^2
std::vector<Reading> xray_series(std::string_view body, int bucket_minutes);
// Every valid row, oldest first (stable for equal times); nullopt when the body is not a JSON array.
std::optional<std::vector<Reading>> kp_history(std::string_view body);
std::optional<std::vector<Reading>> xray_history(std::string_view body); // 0.1-0.8nm only, unbucketed
double xray_plot_fraction(double flux_w_m2);
std::string xray_class(double flux_w_m2);      // 3.2e-6 -> "C3.2"; "" when invalid
std::string kp_description(double kp);         // NOAA G-scale wording

} // namespace hcp::swpc
