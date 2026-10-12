#include "solar_position.hpp"
#include <cmath>
#include <stdexcept>
namespace hamclock {
namespace {
constexpr double kPi = 3.14159265358979323846;
double rad(double d) { return d * kPi / 180.0; }
double deg(double r) { return r * 180.0 / kPi; }
double wrap_longitude(double lon) {
    lon = std::fmod(lon + 180.0, 360.0);
    if (lon < 0) lon += 360.0;
    return lon - 180.0;
}
void require_finite(double lat, double lon) {
    if (!std::isfinite(lat) || !std::isfinite(lon) || lat < -90.0 || lat > 90.0)
        throw std::invalid_argument("invalid location");
}
}
SubsolarPoint subsolar_point(UtcSeconds utc) {
    const double seconds = static_cast<double>(utc.value);
    const double jd = seconds / 86400.0 + 2440587.5;
    const double t = (jd - 2451545.0) / 36525.0;
    const double l0 = rad(std::fmod(280.46646 + t * (36000.76983 + 0.0003032 * t), 360.0));
    const double m = rad(std::fmod(357.52911 + t * (35999.05029 - 0.0001537 * t), 360.0));
    const double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
    const double c = rad(std::sin(m) * (1.914602 - t * (0.004817 + 0.000014 * t)) +
                         std::sin(2 * m) * (0.019993 - 0.000101 * t) + std::sin(3 * m) * 0.000289);
    const double omega = rad(125.04 - 1934.136 * t);
    const double lambda = l0 + c - rad(0.00569) - rad(0.00478) * std::sin(omega);
    const double eps0 = 23.0 + (26.0 + (21.448 - t * (46.815 + t * (0.00059 - t * 0.001813))) / 60.0) / 60.0;
    const double eps = rad(eps0 + 0.00256 * std::cos(omega));
    const double dec = std::asin(std::sin(eps) * std::sin(lambda));
    const double y = std::tan(eps / 2) * std::tan(eps / 2);
    const double eot_min = 4.0 * deg(y * std::sin(2 * l0) - 2 * e * std::sin(m) +
                                      4 * e * y * std::sin(m) * std::cos(2 * l0) -
                                      0.5 * y * y * std::sin(4 * l0) - 1.25 * e * e * std::sin(2 * m));
    const double day_seconds = seconds - 86400.0 * std::floor(seconds / 86400.0);
    const double hours = day_seconds / 3600.0;
    return {deg(dec), wrap_longitude(-15.0 * (hours - 12.0) - eot_min / 4.0)};
}
double solar_elevation_deg(SubsolarPoint sun, double latitude_deg, double longitude_deg) {
    require_finite(latitude_deg, longitude_deg);
    const double lat = rad(latitude_deg), dec = rad(sun.latitude_deg);
    const double h = rad(longitude_deg - sun.longitude_deg);
    double s = std::sin(lat) * std::sin(dec) + std::cos(lat) * std::cos(dec) * std::cos(h);
    if (s > 1.0) s = 1.0;
    if (s < -1.0) s = -1.0;
    return deg(std::asin(s));
}
double terminator_latitude_deg(SubsolarPoint sun, double longitude_deg) {
    require_finite(0.0, longitude_deg);
    const double dec = rad(sun.latitude_deg);
    const double h = rad(longitude_deg - sun.longitude_deg);
    // tan(lat) = -cos(H) / tan(dec); atan2 form stays finite at the equinox.
    double num = -std::cos(h) * std::cos(dec), den = std::sin(dec);
    if (den < 0) { num = -num; den = -den; }
    return deg(std::atan2(num, den));
}
}
