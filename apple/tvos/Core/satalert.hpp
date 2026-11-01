#pragma once
// Qt-free port of the satellite pass alert pieces the TV needs: the curated catalog
// (src/widgets/satellites/sat_catalog, amateur-satellites.json), the selection rules
// (sat_alert_prefs: is_alert_selected / toggle / quiet hours) and the in-app banner
// (sat_alert_plan: alert_passes / alert_body / current_banner). The Qt code is the oracle.
// No notifications: tvOS has no local-notification banner worth using, so the TV shows the
// in-app banner only, as the Qt app does on the Pi.
#include "domain/astro_satellite.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::satalert {

struct FreqRange {
    double lo_mhz = 0, hi_mhz = 0;
    bool valid() const { return lo_mhz > 0 && hi_mhz >= lo_mhz; }
};

struct Transponder {
    std::string mode; // "FM", "Linear", "APRS" or "Digital"
    FreqRange uplink, downlink;
    double ctcss_hz = 0;
};

struct Entry {
    int norad = 0;
    std::string name;   // "SO-50"
    std::string status; // "active", "intermittent" or "inactive"
    std::vector<Transponder> transponders; // never empty
    bool workable() const { return status != "inactive"; }
};

struct Catalog {
    std::vector<Entry> entries;
    const Entry* find(int norad) const;
};

// Malformed entries are skipped, duplicate NORAD ids keep the first; empty when not a JSON object.
Catalog parse_catalog(std::string_view json);

struct Prefs {
    bool enabled = false;
    bool all_workable = true;
    std::vector<int> muted, picked; // sorted, unique, positive
    int min_elevation_deg = 15;
    int lead_minutes = 5;
    int quiet_start_hour = -1, quiet_end_hour = -1; // local hours [start, end), may wrap; -1 none
    bool daylight_only = false;
    bool announce_start = true;
};

Prefs sanitized(Prefs p);
bool is_selected(const Prefs& p, const Catalog& c, int norad);
Prefs toggle(const Prefs& p, const Catalog& c, int norad);
bool in_quiet_hours(const Prefs& p, int local_hour);

struct AlertPass {
    int norad = 0;
    std::string name;
    int64_t aos = 0, tca = 0, los = 0; // unix seconds
    double max_el_deg = 0, sun_el_deg = 0;
    std::string aos_compass, los_compass;
};

// Predicted passes (horizon to horizon) of the selected birds in the TLE text over the next 48 h
// (and up to 3 h back, so a pass in progress is included), sorted by AOS.
std::vector<AlertPass> predict(const std::vector<hamclock::Tle>& tles, const Prefs& p, const Catalog& c,
                               const hamclock::Observer& obs, int64_t now);

// Passes that deserve an alert: selected, high enough, not over, AOS outside quiet hours
// (local = UTC + utc_offset_s), Sun up when daylight only. Sorted by AOS. Empty when off.
std::vector<AlertPass> alert_passes(const std::vector<AlertPass>& passes, const Prefs& p, const Catalog& c,
                                    int utc_offset_s, int64_t now);

// "AOS 14:05 · max 54° · NW → SE · FM 437.800 MHz down, 145.990 up, 67.0 Hz"
std::string alert_body(const AlertPass& pass, const Entry* entry, int utc_offset_s);
// "FM 437.800 MHz down, 145.990 up, 67.0 Hz"
std::string describe(const Transponder& t);

struct Banner {
    bool live = false; // the satellite is up
    std::string text;
};
// The earliest alert pass within lead time ("SO-50 in 4 min · ...") or in progress when
// announce_start ("SO-50 is up now · ...").
std::optional<Banner> current_banner(const std::vector<AlertPass>& passes, const Prefs& p, const Catalog& c,
                                     int utc_offset_s, int64_t now);

// NORAD catalogue number from TLE line 1 (0 when malformed).
int norad_id(const hamclock::Tle& tle);

} // namespace hcp::satalert
