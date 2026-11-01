#include "pathrel.hpp"
#include "domain/solar_position.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace hcp::pathrel {

using hamclock::SubsolarPoint;
using hamclock::solar_elevation_deg;
using hamclock::subsolar_point;
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthKm = 6371.0;
constexpr double kF2HeightKm = 300.0;
constexpr double kDHeightKm = 100.0;
constexpr double kMaxHopKm = 3000.0;
constexpr double kMaxF2HopKm = 4000.0; // P.533 sec 3.5.1: dmax restricted to 4000 km when finding the MUF
constexpr double kMagneticPoleLat = 80.7, kMagneticPoleLon = -72.7; // centred dipole, IGRF-13
constexpr double kAuroralMagLat = 60.0;
constexpr double kMaxSolarIndex = 160.0; // P.533 sec 3.4: R12 saturates here for foF2
constexpr double kKtbDbw = -170.0; // kT0 * 2.5 kHz
struct BandDef { const char* name; double mhz; };
constexpr BandDef kBands[] = {{"80m", 3.6}, {"40m", 7.1}, {"30m", 10.12}, {"20m", 14.2},
                              {"17m", 18.1}, {"15m", 21.2}, {"12m", 24.9}, {"10m", 28.4}};

double rad(double deg) { return deg * kPi / 180.0; }
double deg(double r) { return r * 180.0 / kPi; }
struct Point { double lat, lon; };

// Great-circle angle (radians) and the point a fraction f of the way along it.
double central_angle(Point a, Point b) {
    const double s = std::sin(rad(a.lat)) * std::sin(rad(b.lat)) +
                     std::cos(rad(a.lat)) * std::cos(rad(b.lat)) * std::cos(rad(b.lon - a.lon));
    return std::acos(std::clamp(s, -1.0, 1.0));
}
Point along(Point a, Point b, double fraction) {
    const double d = central_angle(a, b);
    if (d < 1e-9) return a;
    const double s1 = std::sin((1 - fraction) * d) / std::sin(d), s2 = std::sin(fraction * d) / std::sin(d);
    const double x = s1 * std::cos(rad(a.lat)) * std::cos(rad(a.lon)) + s2 * std::cos(rad(b.lat)) * std::cos(rad(b.lon));
    const double y = s1 * std::cos(rad(a.lat)) * std::sin(rad(a.lon)) + s2 * std::cos(rad(b.lat)) * std::sin(rad(b.lon));
    const double z = s1 * std::sin(rad(a.lat)) + s2 * std::sin(rad(b.lat));
    return {deg(std::atan2(z, std::hypot(x, y))), deg(std::atan2(y, x))};
}
double takeoff(double theta, double h) {
    const double top = kEarthKm + h;
    return std::atan2(top * std::cos(theta) - kEarthKm, top * std::sin(theta));
}
// Ray incidence at the D layer for the F2 ray leaving the ground at `takeoff_rad` (ITU-R P.533 i110).
double d_layer_incidence(double takeoff_rad) {
    return std::asin(std::clamp(kEarthKm * std::cos(takeoff_rad) / (kEarthKm + kDHeightKm), 0.0, 0.999));
}
// Noise in 2.5 kHz (dBW): quiet-rural man-made plus galactic noise (ITU-R P.372 Fam), power-summed.
double noise_dbw(double mhz) {
    const double quiet_rural = 53.6 - 28.6 * std::log10(mhz);
    const double galactic = 52.0 - 23.0 * std::log10(mhz);
    const double fam = 10 * std::log10(std::pow(10.0, quiet_rural / 10) + std::pow(10.0, galactic / 10));
    return kKtbDbw + fam;
}
double logistic(double x) { return 1.0 / (1.0 + std::exp(-x)); }

double wrap_longitude(double lon) { return std::fmod(std::fmod(lon + 180.0, 360.0) + 360.0, 360.0) - 180.0; }
double geomagnetic_lat(Point p) {
    const double s = std::sin(rad(p.lat)) * std::sin(rad(kMagneticPoleLat)) +
                     std::cos(rad(p.lat)) * std::cos(rad(kMagneticPoleLat)) * std::cos(rad(p.lon - kMagneticPoleLon));
    return deg(std::asin(std::clamp(s, -1.0, 1.0)));
}
// F2 layer at a control point: a compact fit to the monthly median CCIR maps behind ITU-R P.1239
// (all months, 5 solar levels, global; see the header for the error against them).
struct F2Layer { double fo_mhz, m3000, season, mag_lat, daylight; };
F2Layer f2_layer(double sfi, SubsolarPoint sun, Point p) {
    const double r12 = std::sqrt(std::max(0.0, 167273.0 + (sfi - 63.7) * 1123.6)) - 408.99; // P.371
    const double activity = std::clamp(r12, 0.0, kMaxSolarIndex) / 100.0;
    const double mag_lat = std::abs(geomagnetic_lat(p));
    const double season = sun.latitude_deg / 23.44 * std::clamp(p.lat / 30.0, -1.0, 1.0); // +1 local summer, -1 winter
    const double crest = std::exp(-std::pow((mag_lat - 11.11) / 24.0, 2)); // equatorial anomaly
    // The layer lags the sun by about 50 minutes and fills and drains gradually around sunrise and sunset.
    const double daylight = logistic((solar_elevation_deg(sun, p.lat, p.lon - 12.45) + 1.49) / 10.0);
    const double local_hour = 12.0 + (p.lon - sun.longitude_deg) / 15.0;
    const double day_mhz = (5.99 + 3.16 * activity) * (1 + 0.33 * crest) * (1 - 0.05 * season * (1 + 2.07 * activity)) *
                           (1 - 0.33 * std::pow(mag_lat / 90.0, 2)); // winter anomaly: more by day in winter
    const double night_mhz = (1.95 + 1.61 * activity) * (1 + 0.99 * crest) * (1 + 0.28 * season) *
                             (1 + 0.37 * std::cos(2 * kPi * (local_hour - 19.99) / 24.0)); // decays to a pre-dawn low
    const double m3000 = 3.23 - 0.32 * activity + daylight * (-0.06 - 0.24 * season);
    return {night_mhz + (day_mhz - night_mhz) * daylight, m3000, season, mag_lat, daylight};
}
// F2 basic MUF for a hop of d km, P.533 eqs 3-6 with dmax at its 4000 km limit and foF2/foE = 3.
double f2_basic_muf(const F2Layer& layer, double d) {
    const auto distance_term = [](double km) {
        const double z = 1 - 2 * km / kMaxF2HopKm;
        return 0.74 + z * (-0.591 + z * (-0.424 + z * (-0.090 + z * (0.088 + z * (0.181 + z * 0.096)))));
    };
    d = std::min(d, kMaxF2HopKm);
    const double b = layer.m3000 - 0.124 + (layer.m3000 * layer.m3000 - 4) * 0.0245;
    const double gyro = 0.87 * std::pow(kEarthKm / (kEarthKm + kF2HeightKm), 3) *
                        std::sqrt(1 + 3 * std::pow(std::sin(rad(layer.mag_lat)), 2));
    return (1 + distance_term(d) / distance_term(3000.0) * (b - 1)) * layer.fo_mhz + gyro / 2 * (1 - d / kMaxF2HopKm);
}
// E layer basic MUF for a hop of d <= 2000 km, P.1239 eqs 12-18 and P.1240 sec 5.1. Zero after dark.
double e_basic_muf(double sfi, SubsolarPoint sun, Point p, double d) {
    const double elevation = solar_elevation_deg(sun, p.lat, p.lon);
    if (elevation <= 0) return 0;
    const double chi = 90.0 - elevation, cos_lat = std::cos(rad(p.lat));
    const bool tropics = std::abs(p.lat) < 32.0;
    const double a = std::max(0.0, 1 + 0.0094 * (sfi - 66));
    const double b = std::pow(std::cos(rad(std::min(std::abs(p.lat - sun.latitude_deg), 80.0))),
                              tropics ? -1.93 + 1.92 * cos_lat : 0.11 - 0.49 * cos_lat);
    const double c = tropics ? 23 + 116 * cos_lat : 92 + 35 * cos_lat;
    const double bent = chi > 73 ? chi - 6.27e-13 * std::pow(chi - 50, 8) : chi;
    const double fo_e = std::pow(a * b * c * std::pow(std::cos(rad(bent)), 1.2), 0.25);
    const double x = (std::min(d, 2000.0) - 1150.0) / 1150.0;
    return fo_e * (3.94 + 2.80 * x - 1.70 * x * x - 0.60 * x * x * x + 0.96 * x * x * x * x);
}
// Ratio of operational to basic F2 MUF, P.1240 Table 1: season, day to night, and EIRP.
double operational_ratio(double season, double daylight, double eirp_dbw) {
    return 1.25 - 0.10 * daylight - 0.05 * season + (eirp_dbw >= 30 ? 0.05 : 0.0);
}
// P(the operational MUF supports f on a given day of the month), P.533 eqs 9-10. The decile ratios
// are the mid-latitude values of P.1239 Tables 2 and 3, night to day.
double muf_support(double mhz, double muf, double daylight) {
    const double lower = 0.78 + 0.07 * daylight, upper = 1.25 - 0.12 * daylight;
    if (mhz <= muf) return std::min(1.0, 1.3 - 0.8 / (1 + (1 - mhz / muf) / (1 - lower)));
    return std::max(0.0, 0.8 / (1 + (mhz / muf - 1) / (upper - 1)) - 0.3);
}
double need_db(PathMode mode) {
    switch (mode) {
    case PathMode::Cw: return 5;
    case PathMode::Digital: return -15;
    case PathMode::Ssb: break;
    }
    return 10;
}
} // namespace

double muf_3000_mhz(double sfi, double lat, double lon, UtcSeconds utc) {
    lat = std::clamp(lat, -90.0, 90.0);
    lon = std::fmod(lon + 540.0, 360.0) - 180.0;
    const auto layer = f2_layer(std::isfinite(sfi) ? std::clamp(sfi, 60.0, 300.0) : 60.0, subsolar_point(utc), {lat, lon});
    return layer.fo_mhz * layer.m3000;
}

double path_distance_km(const PathInput& p) {
    const double angle = central_angle({p.de_lat, p.de_lon}, {p.dx_lat, p.dx_lon});
    return (p.long_path ? 2 * kPi - angle : angle) * kEarthKm;
}

std::vector<PathBand> path_outlook(const PathInput& p, UtcSeconds utc, double sfi, double kp) {
    sfi = std::isfinite(sfi) ? std::clamp(sfi, 60.0, 300.0) : 60.0;
    kp = std::isfinite(kp) ? std::clamp(kp, 0.0, 9.0) : 0.0;
    const Point de{p.de_lat, p.de_lon}, dx{p.dx_lat, p.dx_lon};
    const double dist = std::max(path_distance_km(p), 50.0);
    const int hops = std::max(1, static_cast<int>(std::ceil(dist / kMaxHopKm)));
    const double hop_km = dist / hops;
    const double theta = hop_km / (2 * kEarthKm);
    const double sec_d = 1.0 / std::cos(d_layer_incidence(takeoff(theta, kF2HeightKm)));
    const double r12 = std::max(0.0, (sfi - 67) / 0.7);
    const auto sun = subsolar_point(utc);

    const double short_angle = central_angle(de, dx);
    const auto point_at_km = [&](double km) { // along the rated path from DE; long path continues away from DX
        const double t = km / dist;
        const Point c = p.long_path && short_angle > 1e-9 ? along(de, dx, -t * (2 * kPi - short_angle) / short_angle)
                                                          : along(de, dx, t);
        return Point{std::clamp(c.lat, -90.0, 90.0), wrap_longitude(c.lon)};
    };
    double auroral = std::abs(geomagnetic_lat(point_at_km(dist / 2))); // highest geomagnetic latitude sampled
    std::vector<double> zenith;
    for (int i = 0; i < hops; ++i) {
        const Point c = point_at_km(dist * (i + 0.5) / hops);
        zenith.push_back(90.0 - solar_elevation_deg(sun, c.lat, c.lon));
        auroral = std::max(auroral, std::abs(geomagnetic_lat(c)));
    }
    // Path MUF, P.533 Table 1a and P.1240 sec 3.2: mid-path up to dmax, beyond it the lower
    // F2(dmax)MUF of the two points half a lowest-order hop from each end. Mid-path darkness on a
    // long path does not set the MUF. The operational MUF scales the F2 value only (P.1240 sec 6).
    const double power_dbw = 10 * std::log10(std::max(p.power_w, 0.01));
    const int f2_hops = static_cast<int>(std::ceil(dist / kMaxF2HopKm));
    const double f2_hop_km = dist / f2_hops;
    std::vector<Point> f2_points{point_at_km(dist / 2)};
    if (f2_hops > 1) f2_points = {point_at_km(f2_hop_km / 2), point_at_km(dist - f2_hop_km / 2)};
    double muf = 1e9, daylight = 1;
    for (const Point c : f2_points) {
        const auto layer = f2_layer(sfi, sun, c);
        const double f2 = f2_basic_muf(layer, f2_hops > 1 ? kMaxF2HopKm : dist) *
                          operational_ratio(layer.season, layer.daylight, power_dbw + p.gain_db / 2);
        if (f2 < muf) { muf = f2; daylight = layer.daylight; }
    }
    if (dist <= kMaxF2HopKm) { // E modes: the higher of E and F2 gives the path MUF (P.533 sec 3.1)
        const double e = dist <= 2000.0 ? e_basic_muf(sfi, sun, point_at_km(dist / 2), dist)
                                        : std::min(e_basic_muf(sfi, sun, point_at_km(1000.0), 2000.0),
                                                   e_basic_muf(sfi, sun, point_at_km(dist - 1000.0), 2000.0));
        muf = std::max(muf, e);
    }
    const double slant_km = hops * 2 * std::sqrt(kEarthKm * kEarthKm + std::pow(kEarthKm + kF2HeightKm, 2) -
                                                  2 * kEarthKm * (kEarthKm + kF2HeightKm) * std::cos(theta));
    double storm = kp >= 7 ? 0.3 : kp >= 5 ? 0.7 : 1.0;
    if (auroral > kAuroralMagLat && kp >= 3) storm *= std::max(0.3, 1.0 - 0.1 * kp);

    std::vector<PathBand> out;
    for (const auto& def : kBands) {
        double absorb = 0;
        for (double z : zenith) {
            const double i_day = z < 100 ? std::pow(std::max(0.0, std::cos(rad(0.881 * std::min(z, 90.0)))), 1.3) : 0.0;
            absorb += 0.6 * 677.2 * (1 + 0.0037 * r12) * i_day * sec_d / (std::pow(def.mhz + 1.0, 1.98) + 10.2);
        }
        const double fspl = 32.45 + 20 * std::log10(def.mhz) + 20 * std::log10(slant_km);
        const double noise = noise_dbw(def.mhz);
        const double snr = power_dbw + p.gain_db - fspl - absorb - 2.0 * (hops - 1) - noise;
        const double rel = muf_support(def.mhz, muf, daylight) * logistic((snr - need_db(p.mode)) / 4.0) * storm;
        out.push_back({def.name, def.mhz, muf, snr, std::clamp(rel, 0.0, 1.0), deg(takeoff(theta, kF2HeightKm)), hops});
    }
    return out;
}

std::vector<std::vector<double>> path_matrix(const PathInput& p, UtcSeconds start, int hours, double sfi, double kp) {
    std::vector<std::vector<double>> rows(std::size(kBands));
    for (int h = 0; h < hours; ++h) {
        const auto column = path_outlook(p, UtcSeconds{start.value + 3600LL * h}, sfi, kp);
        for (std::size_t b = 0; b < column.size(); ++b) rows[b].push_back(column[b].reliability);
    }
    return rows;
}

Card matrix_card(const PathInput& path, int64_t now_s, double sfi, double kp, int hours) {
    Card card;
    PathInput short_path = path;
    short_path.long_path = false;
    card.has_path = path_distance_km(short_path) >= 50; // DX not set (or on top of DE): no path to rate
    if (!card.has_path || hours <= 0) return card;
    const int64_t hour = (now_s >= 0 ? now_s : now_s - 3599) / 3600;
    const UtcSeconds start{hour * 3600};
    card.distance_km = path_distance_km(path);
    const auto first = path_outlook(path, start, sfi, kp);
    card.cells = path_matrix(path, start, hours, sfi, kp);
    std::size_t best = 0;
    for (std::size_t b = 0; b < first.size(); ++b) {
        card.bands.push_back(first[b].band);
        card.muf_mhz.push_back(first[b].muf_mhz);
        if (first[b].reliability > first[best].reliability) best = b;
    }
    card.best_text = first[best].reliability < 0.05
                         ? std::string("HF closed now")
                         : "Best now: " + first[best].band + " " +
                               std::to_string(static_cast<int>(std::lround(first[best].reliability * 100))) + "%";
    char hh[8];
    std::snprintf(hh, sizeof hh, "%02dZ", static_cast<int>(((hour % 24) + 24) % 24));
    card.start_text = hh;
    return card;
}

} // namespace hcp::pathrel
