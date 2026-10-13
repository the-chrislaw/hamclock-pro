#include "astro_satellite.hpp"
#include "astro_sgp4/SGP4.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

namespace hamclock {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg = kPi / 180.0;
constexpr double kA = 6378.137;            // WGS84 equatorial radius, km
constexpr double kF = 1.0 / 298.257223563; // WGS84 flattening

double julian(double unix_time) { return unix_time / 86400.0 + 2440587.5; }

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

void ecef_of_geodetic(double lat, double lon, double alt_km, double out[3]) {
    const double e2 = kF * (2 - kF);
    const double n = kA / std::sqrt(1 - e2 * std::sin(lat) * std::sin(lat));
    out[0] = (n + alt_km) * std::cos(lat) * std::cos(lon);
    out[1] = (n + alt_km) * std::cos(lat) * std::sin(lon);
    out[2] = (n * (1 - e2) + alt_km) * std::sin(lat);
}
} // namespace

std::vector<Tle> parse_tle_set(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    for (std::string l; std::getline(in, l);) {
        l = trim(l);
        if (!l.empty()) lines.push_back(l);
    }
    std::vector<Tle> out;
    for (std::size_t i = 0; i < lines.size();) {
        const bool l1 = lines[i].size() >= 69 && lines[i][0] == '1' && lines[i][1] == ' ';
        if (l1 && i + 1 < lines.size() && lines[i + 1].size() >= 69 && lines[i + 1][0] == '2') {
            out.push_back({"", lines[i], lines[i + 1]});
            i += 2;
        } else if (!l1 && i + 2 < lines.size() && lines[i + 1].size() >= 69 && lines[i + 1][0] == '1'
                   && lines[i + 2].size() >= 69 && lines[i + 2][0] == '2') {
            out.push_back({lines[i], lines[i + 1], lines[i + 2]});
            i += 3;
        } else {
            ++i;
        }
    }
    return out;
}

struct SatellitePropagator::Impl {
    elsetrec rec{};
    bool ok = false;
    double jd_epoch = 0;
};

SatellitePropagator::SatellitePropagator(const Tle& tle) : impl_(new Impl) {
    char l1[130] = {}, l2[130] = {};
    std::strncpy(l1, tle.line1.c_str(), 129);
    std::strncpy(l2, tle.line2.c_str(), 129);
    double a = 0, b = 0, c = 0;
    SGP4Funcs::twoline2rv(l1, l2, 'c', 'e', 'a', wgs72, a, b, c, impl_->rec);
    impl_->ok = impl_->rec.error == 0;
    impl_->jd_epoch = impl_->rec.jdsatepoch + impl_->rec.jdsatepochF;
}
SatellitePropagator::~SatellitePropagator() = default;
SatellitePropagator::SatellitePropagator(SatellitePropagator&&) noexcept = default;
SatellitePropagator& SatellitePropagator::operator=(SatellitePropagator&&) noexcept = default;

bool SatellitePropagator::valid() const { return impl_->ok; }
double SatellitePropagator::epoch_unix() const { return (impl_->jd_epoch - 2440587.5) * 86400.0; }

bool SatellitePropagator::teme(double unix_time, double r[3]) const {
    if (!impl_->ok) return false;
    double v[3];
    const double tsince = (julian(unix_time) - impl_->jd_epoch) * 1440.0;
    return SGP4Funcs::sgp4(impl_->rec, tsince, r, v);
}

namespace {
// TEME -> ECEF by GMST rotation (polar motion ignored: well under 0.1 km).
void ecef_of_teme(const double r[3], double unix_time, double out[3]) {
    const double g = SGP4Funcs::gstime_SGP4(julian(unix_time));
    out[0] = std::cos(g) * r[0] + std::sin(g) * r[1];
    out[1] = -std::sin(g) * r[0] + std::cos(g) * r[1];
    out[2] = r[2];
}
} // namespace

bool SatellitePropagator::sub_point(double t, SubSatellitePoint& out) const {
    double r[3], e[3];
    if (!teme(t, r)) return false;
    ecef_of_teme(r, t, e);
    const double e2 = kF * (2 - kF);
    const double p = std::hypot(e[0], e[1]);
    double lat = std::atan2(e[2], p * (1 - e2)), alt = 0;
    for (int i = 0; i < 6; ++i) {
        const double n = kA / std::sqrt(1 - e2 * std::sin(lat) * std::sin(lat));
        alt = p / std::cos(lat) - n;
        lat = std::atan2(e[2], p * (1 - e2 * n / (n + alt)));
    }
    out.lat_deg = lat / kDeg;
    out.lon_deg = std::remainder(std::atan2(e[1], e[0]) / kDeg, 360.0);
    out.alt_km = alt;
    return true;
}

bool SatellitePropagator::look(double t, const Observer& obs, LookAngle& out) const {
    double r[3], e[3], o[3];
    if (!teme(t, r)) return false;
    ecef_of_teme(r, t, e);
    const double lat = obs.lat_deg * kDeg, lon = obs.lon_deg * kDeg;
    ecef_of_geodetic(lat, lon, obs.alt_km, o);
    const double d[3] = {e[0] - o[0], e[1] - o[1], e[2] - o[2]};
    const double east = -std::sin(lon) * d[0] + std::cos(lon) * d[1];
    const double north = -std::sin(lat) * std::cos(lon) * d[0] - std::sin(lat) * std::sin(lon) * d[1] + std::cos(lat) * d[2];
    const double up = std::cos(lat) * std::cos(lon) * d[0] + std::cos(lat) * std::sin(lon) * d[1] + std::sin(lat) * d[2];
    out.range_km = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    out.el_deg = std::asin(up / out.range_km) / kDeg;
    out.az_deg = std::fmod(std::atan2(east, north) / kDeg + 360.0, 360.0);
    return true;
}

std::vector<SatPass> SatellitePropagator::passes(const Observer& obs, double from, double horizon_s,
                                                 double min_el, int max_passes) const {
    std::vector<SatPass> out;
    if (!impl_->ok) return out;
    const double step = 20.0;
    auto elev = [&](double t) {
        LookAngle la;
        return look(t, obs, la) ? la.el_deg : -90.0;
    };
    // Bisect the el == min_el crossing between a (below/above) and b.
    auto cross = [&](double a, double b) {
        const bool a_up = elev(a) >= min_el;
        for (int i = 0; i < 20; ++i) {
            const double m = 0.5 * (a + b);
            ((elev(m) >= min_el) == a_up ? a : b) = m;
        }
        return 0.5 * (a + b);
    };
    bool up = elev(from) >= min_el;
    SatPass cur;
    double aos = from;
    bool in_pass = up;
    double best_t = from, best_el = up ? elev(from) : -90;
    for (double t = from + step; t <= from + horizon_s && static_cast<int>(out.size()) < max_passes; t += step) {
        const double el = elev(t);
        const bool now_up = el >= min_el;
        if (now_up && !in_pass) {
            aos = cross(t - step, t);
            in_pass = true;
            best_el = el;
            best_t = t;
        } else if (now_up) {
            if (el > best_el) { best_el = el; best_t = t; }
        } else if (in_pass) {
            cur.aos = static_cast<std::int64_t>(std::llround(aos));
            cur.los = static_cast<std::int64_t>(std::llround(cross(t - step, t)));
            // Refine the peak around the best sample.
            double lo = best_t - step, hi = best_t + step;
            for (int i = 0; i < 30; ++i) {
                const double m1 = lo + (hi - lo) / 3, m2 = hi - (hi - lo) / 3;
                if (elev(m1) < elev(m2)) lo = m1; else hi = m2;
            }
            cur.tca = static_cast<std::int64_t>(std::llround(0.5 * (lo + hi)));
            cur.max_el_deg = elev(0.5 * (lo + hi));
            out.push_back(cur);
            in_pass = false;
            best_el = -90;
        }
    }
    return out;
}

} // namespace hamclock
