#pragma once
// Path-specific HF reliability estimate for a DE -> DX pair. This is our own
// compact, offline model (item B13 option C): it is NOT VOACAP or ITU-R P.533
// and is labelled approximate wherever it is shown. Method, per band:
//   * Path is split into equal F2 hops of at most 3000 km; each hop's midpoint
//     is a control point.
//   * foF2 at a control point: the noon value 5.5 + 0.06 * (SFI - 70) MHz,
//     blended toward 45% of it at night by sin(solar elevation)^0.6, and
//     reduced up to 40% toward the poles. MUF = foF2 / cos(incidence angle at
//     300 km); the path MUF is the lowest control-point MUF.
//   * Absorption: ITU-R P.533-style D-layer loss per hop (scaled by 0.6, tuned so a daytime
//     transatlantic 20m path loses about 10 dB), by solar zenith angle, summed.
//   * SNR in 2.5 kHz = power + gain - free-space loss over the slanted path -
//     absorption - 2 dB per extra ground reflection - quiet-rural noise
//     (ITU-R P.372). Mode sets the SNR needed.
//   * Reliability = P(f below the median MUF, logistic, +-6%) * P(SNR above
//     the need, logistic, 4 dB) * geomagnetic factor (Kp >= 5 and polar paths).
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
