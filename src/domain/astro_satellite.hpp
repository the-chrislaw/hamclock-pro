// Satellite tracking: TLE parsing, SGP4 propagation (vendored Vallado code),
// sub-satellite point, look angles and next-pass search. Offline.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hamclock {

struct Tle {
    std::string name;
    std::string line1;
    std::string line2;
};

// Parses 3-line (name + two lines) or bare 2-line TLE text; skips malformed sets.
std::vector<Tle> parse_tle_set(const std::string& text);

struct SubSatellitePoint { double lat_deg = 0, lon_deg = 0, alt_km = 0; };
struct LookAngle { double az_deg = 0, el_deg = 0, range_km = 0, range_rate_km_s = 0; }; // rate > 0: receding
struct Observer { double lat_deg = 0, lon_deg = 0, alt_km = 0; };

// Doppler-corrected frequency (Hz) seen at the observer for a transmission of
// `f_hz` from the satellite: f * (1 - rate/c). Positive range rate lowers it.
double doppler_downlink_hz(double f_hz, double range_rate_km_s);
// Frequency to transmit (Hz) so the satellite receives `f_hz`: f * (1 + rate/c).
double doppler_uplink_hz(double f_hz, double range_rate_km_s);

struct SatPass {
    std::int64_t aos = 0, tca = 0, los = 0; // unix seconds
    double max_el_deg = 0;
};

// One az/el sample of a pass, for the sky plot.
struct PassSample { std::int64_t t = 0; double az_deg = 0, el_deg = 0; };

class SatellitePropagator {
public:
    explicit SatellitePropagator(const Tle& tle);
    ~SatellitePropagator();
    SatellitePropagator(SatellitePropagator&&) noexcept;
    SatellitePropagator& operator=(SatellitePropagator&&) noexcept;

    bool valid() const;
    double epoch_unix() const;
    // Position in the TEME frame, km; false if SGP4 reports an error (e.g. decayed).
    bool teme(double unix_time, double r[3]) const;
    bool sub_point(double unix_time, SubSatellitePoint& out) const;
    bool look(double unix_time, const Observer& obs, LookAngle& out) const;
    // Passes above min_el_deg starting after `from`, within the horizon, up to max_passes.
    std::vector<SatPass> passes(const Observer& obs, double from, double horizon_s,
                                double min_el_deg, int max_passes) const;

    // Az/el samples from AOS to LOS inclusive, at most `max_samples` (>= 2) evenly spaced.
    std::vector<PassSample> pass_track(const Observer& obs, const SatPass& pass, int max_samples) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace hamclock
