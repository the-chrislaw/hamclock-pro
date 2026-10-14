// Satellite transmitter records from the SatNOGS DB (CC BY-SA 4.0), joined to
// TLEs by NORAD id. Parsing and frequency maths only; offline.
#pragma once
#include "astro_satellite.hpp"
#include <map>
#include <string>
#include <vector>

namespace hamclock {

struct Transmitter {
    int norad_id = 0;
    std::string description;
    std::string mode;        // downlink mode, e.g. FM, USB, CW
    std::string uplink_mode; // empty if none
    // Hz; 0 means absent. *_high_hz is 0 for a single frequency.
    double downlink_low_hz = 0, downlink_high_hz = 0;
    double uplink_low_hz = 0, uplink_high_hz = 0;
    bool inverting = false;
    bool active = false;   // database status "active"
    double ctcss_hz = 0;   // 0 unless the description states a tone ("CTCSS 67.0")
    bool transponder() const { return downlink_high_hz > downlink_low_hz && uplink_high_hz > uplink_low_hz; }
};

// Parses the JSON array from /api/transmitters/?format=json. Entries without a
// NORAD id or without any frequency are skipped; malformed input gives {}.
std::vector<Transmitter> parse_satnogs_transmitters(const std::string& json);

// NORAD catalogue number from TLE line 1 (columns 3-7); 0 if unreadable.
int tle_norad_id(const Tle& tle);

std::map<int, std::vector<Transmitter>> group_by_norad(const std::vector<Transmitter>& all);

// Downlink frequency (Hz, before Doppler) for a given uplink frequency through a
// linear transponder; 0 if `t` is not a transponder or the uplink is outside its
// passband. Inverting transponders reverse the sense of the offset.
double transponder_downlink_hz(const Transmitter& t, double uplink_hz);

} // namespace hamclock
