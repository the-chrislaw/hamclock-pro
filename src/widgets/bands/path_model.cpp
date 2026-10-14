#include "path_model.hpp"
#include "../../domain/solar_position.hpp"
#include <algorithm>
#include <cmath>

namespace hamclock::bands {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthKm = 6371.0;
constexpr double kF2HeightKm = 300.0;
constexpr double kDHeightKm = 100.0;
constexpr double kMaxHopKm = 3000.0;
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
// Incidence angle at a layer of height h for a hop spanning angle 2*theta.
double incidence(double theta, double h) {
    const double top = kEarthKm + h;
    const double len = std::sqrt(kEarthKm * kEarthKm + top * top - 2 * kEarthKm * top * std::cos(theta));
    return std::asin(std::clamp(kEarthKm * std::sin(theta) / len, 0.0, 0.999));
}
double takeoff(double theta, double h) {
    const double top = kEarthKm + h;
    return std::atan2(top * std::cos(theta) - kEarthKm, top * std::sin(theta));
}
double logistic(double x) { return 1.0 / (1.0 + std::exp(-x)); }

double fo_f2(double sfi, double elevation_deg, double lat) {
    const double noon = 5.5 + 0.06 * (sfi - 70);
    const double day = std::pow(std::max(0.0, std::sin(rad(elevation_deg))), 0.6);
    const double value = noon * (0.45 + 0.55 * day);
    return value * (1.0 - 0.4 * std::pow(std::sin(rad(lat)), 2));
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

double path_distance_km(const PathInput& p) {
    return central_angle({p.de_lat, p.de_lon}, {p.dx_lat, p.dx_lon}) * kEarthKm;
}

std::vector<PathBand> path_outlook(const PathInput& p, UtcSeconds utc, double sfi, double kp) {
    sfi = std::isfinite(sfi) ? std::clamp(sfi, 60.0, 300.0) : 60.0;
    kp = std::isfinite(kp) ? std::clamp(kp, 0.0, 9.0) : 0.0;
    const Point de{p.de_lat, p.de_lon}, dx{p.dx_lat, p.dx_lon};
    const double dist = std::max(path_distance_km(p), 50.0);
    const int hops = std::max(1, static_cast<int>(std::ceil(dist / kMaxHopKm)));
    const double hop_km = dist / hops;
    const double theta = hop_km / (2 * kEarthKm);
    const double sec_f2 = 1.0 / std::cos(incidence(theta, kF2HeightKm));
    const double sec_d = 1.0 / std::cos(incidence(theta, kDHeightKm));
    const double r12 = std::max(0.0, (sfi - 67) / 0.7);
    const auto sun = subsolar_point(utc);

    double muf = 1e9, polar = 0;
    std::vector<double> zenith;
    for (int i = 0; i < hops; ++i) {
        const Point c = along(de, dx, (i + 0.5) / hops);
        const double elev = solar_elevation_deg(sun, std::clamp(c.lat, -90.0, 90.0), std::fmod(c.lon + 540.0, 360.0) - 180.0);
        muf = std::min(muf, fo_f2(sfi, elev, c.lat) * sec_f2);
        zenith.push_back(90.0 - elev);
        polar = std::max(polar, std::abs(c.lat));
    }
    const double slant_km = hops * 2 * std::sqrt(kEarthKm * kEarthKm + std::pow(kEarthKm + kF2HeightKm, 2) -
                                                  2 * kEarthKm * (kEarthKm + kF2HeightKm) * std::cos(theta));
    const double power_dbw = 10 * std::log10(std::max(p.power_w, 0.01)) ;
    double storm = kp >= 7 ? 0.3 : kp >= 5 ? 0.7 : 1.0;
    if (polar > 60 && kp >= 3) storm *= std::max(0.3, 1.0 - 0.1 * kp);

    std::vector<PathBand> out;
    for (const auto& def : kBands) {
        double absorb = 0;
        for (double z : zenith) {
            const double i_day = z < 100 ? std::pow(std::max(0.0, std::cos(rad(0.881 * std::min(z, 90.0)))), 1.3) : 0.0;
            absorb += 0.6 * 677.2 * (1 + 0.0037 * r12) * i_day * sec_d / (std::pow(def.mhz + 1.0, 1.98) + 10.2);
        }
        const double fspl = 32.45 + 20 * std::log10(def.mhz) + 20 * std::log10(slant_km);
        const double noise = -170 + (67.2 - 27.7 * std::log10(def.mhz));
        const double snr = power_dbw + p.gain_db - fspl - absorb - 2.0 * (hops - 1) - noise;
        const double rel = logistic((muf - def.mhz) / (0.06 * def.mhz) ) * logistic((snr - need_db(p.mode)) / 4.0) * storm;
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

} // namespace hamclock::bands
