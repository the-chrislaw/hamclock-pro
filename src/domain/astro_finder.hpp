// "Find it in the sky": pointing model and pass schedule on top of the SGP4
// wrapper. Plain C++ with no sensors or Qt, so the phone finder screens only bind
// compass/motion/camera to it. Offline.
#pragma once
#include "astro_satellite.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace hamclock {

// 16-point compass letters ("N", "NNE", ...) for an azimuth in degrees.
std::string compass_letters(double az_deg);

// Topocentric pointing of a satellite at one instant (the 1 Hz finder feed).
struct PointingState {
    double t = 0;                  // unix seconds
    double az_deg = 0, el_deg = 0; // el < 0: below the horizon
    double range_km = 0, range_rate_km_s = 0; // rate > 0: receding
    bool above_horizon = false;
    bool sunlit = false;
};
std::optional<PointingState> pointing_at(const SatellitePropagator& sat, const Observer& obs, double unix_time);

// Doppler-corrected downlink and uplink for a nominal pair of frequencies.
struct DopplerPair { double downlink_hz = 0, uplink_hz = 0; };
DopplerPair doppler_for(const PointingState& p, double downlink_hz, double uplink_hz);

// A pass with the details a schedule row and a finder screen need.
struct FinderPass {
    SatPass pass;
    double aos_az_deg = 0, tca_az_deg = 0, los_az_deg = 0;
    std::string aos_compass, tca_compass, los_compass;
    std::int64_t duration_s = 0;
    // Sat above the horizon and sunlit while the Sun is below -6 deg at the observer.
    bool visible_to_eye = false;
    // Sun elevation at the observer at TCA, degrees.
    double sun_el_at_tca_deg = 0;
};
FinderPass describe_pass(const SatellitePropagator& sat, const Observer& obs, const SatPass& pass);

// Where to look before the satellite rises: the azimuth/compass of AOS at the
// horizon (elevation 0 even when the schedule uses a higher cut-off).
struct RisePoint { std::int64_t t = 0; double az_deg = 0; std::string compass; };
std::optional<RisePoint> rise_point(const SatellitePropagator& sat, const Observer& obs, double after_unix,
                                    double horizon_s);

struct ScheduledPass { std::string name; FinderPass pass; };
enum class PassFilter { All, VisibleToEye };
// Passes of every satellite in [from, from + horizon_s), sorted by AOS. `include`
// (optional) keeps only satellites it accepts by name, e.g. the Amateur filter
// (has SatNOGS transmitters); it combines with `filter`.
std::vector<ScheduledPass> pass_schedule(const std::vector<std::pair<std::string, const SatellitePropagator*>>& sats,
                                         const Observer& obs, double from, double horizon_s, double min_el_deg,
                                         PassFilter filter, int max_passes,
                                         const std::function<bool(const std::string&)>& include = {});

} // namespace hamclock
