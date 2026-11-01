#pragma once
// Path-specific HF reliability estimate for a DE -> DX pair. This is our own
// compact, offline model (item B13 option C): it is NOT VOACAP or ITU-R P.533
// and is labelled approximate wherever it is shown. Method, per band:
//   * Signal: the path is split into equal F2 hops of at most 3000 km; each hop's midpoint is a
//     point for the loss sums below.
//   * foF2 and M(3000)F2 at a control point: a compact fit to the CCIR maps of ITU-R P.1239 in
//     SFI, season, geomagnetic latitude and time of day. Against those maps (all months, SFI 72
//     to 203, |lat| <= 75) foF2 is within 17% rms with no bias, M(3000)F2 within 0.2.
//   * Path MUF, after P.533 and P.1240: up to 4000 km, F2 MUF at mid-path and E MUF for short
//     hops, the higher wins. Beyond that, F2(4000)MUF at the two points half a lowest-order hop
//     from each end, the lower wins; the middle of a long path does not set it. F2 values are
//     raised to the operational MUF by the P.1240 Table 1 ratio.
//   * Absorption: ITU-R P.533-style D-layer loss per hop (scaled by 0.6, tuned so a daytime
//     transatlantic 20m path loses about 10 dB), by solar zenith angle, summed. The secant is
//     taken where the F2 ray crosses the D layer (P.533 i110).
//   * SNR in 2.5 kHz = power + gain - free-space loss over the slanted path -
//     absorption - 2 dB per extra ground reflection - noise (ITU-R P.372 quiet-rural
//     man-made plus galactic, power-summed). Mode sets the SNR needed.
//   * Reliability = P(MUF supports f on a day of the month, P.533 eqs 9-10) * P(SNR above the
//     need, logistic, 4 dB) * geomagnetic factor (Kp >= 5, and Kp >= 3 for paths reaching 60
//     degrees geomagnetic). It lacks P.533's long-path field model, antenna patterns and E-layer
//     screening, and takes SFI as if it were the smoothed index.
// Short path by default, or the long path (other way round the globe) with `long_path`; takeoff angle is the ground elevation of the first hop.
#include "../../domain/clock_state.hpp"
#include <QString>
#include <vector>

namespace hamclock::bands {

enum class PathMode { Ssb, Cw, Digital };

struct PathInput {
    double de_lat = 0, de_lon = 0, dx_lat = 0, dx_lon = 0; // degrees
    double power_w = 100;
    double gain_db = 6; // antenna gain over isotropic, both ends combined (two simple dipoles)
    PathMode mode = PathMode::Ssb;
    bool long_path = false; // the other way round the globe (2*pi*R minus the short path)
};
struct PathBand {
    QString band;
    double mhz = 0;
    double muf_mhz = 0;
    double snr_db = 0;
    double reliability = 0; // 0..1
    double takeoff_deg = 0;
    int hops = 0;
};

// Length of the path being rated (short, or long when long_path is set).
double path_distance_km(const PathInput& path);
std::vector<PathBand> path_outlook(const PathInput& path, UtcSeconds utc, double sfi, double kp);
// One row per band, one column per hour starting at start_utc (reliability 0..1).
std::vector<std::vector<double>> path_matrix(const PathInput& path, UtcSeconds start_utc, int hours,
                                             double sfi, double kp);

// Single-hop (3000 km) MUF at a point from the same foF2 model, in MHz; the basis of the MUF map style.
double muf_3000_mhz(double sfi, double lat, double lon, UtcSeconds utc);

} // namespace hamclock::bands
