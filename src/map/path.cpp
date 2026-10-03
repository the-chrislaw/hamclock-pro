#include "path.hpp"
#include <cmath>
#include <stdexcept>
namespace hamclock {
namespace {
constexpr double pi = 3.1415926535897932384626433832795;
double rad(double degrees) { return degrees * pi / 180.0; }
}
std::vector<Location> map_short_path(Location de, Location dx, unsigned subdivisions) {
    if (subdivisions < 1 || subdivisions > 512)
        throw std::invalid_argument("map path subdivisions must be 1..512");
    const auto route = geometry(de, dx); // Validates both endpoints and degeneracies.
    if (route.status == BearingStatus::Coincident) return {de};
    if (route.status == BearingStatus::Antipodal)
        throw std::invalid_argument("antipodal map path has no unique route");
    const double lat = rad(de.latitude), lon = rad(de.longitude);
    const double other_lat = rad(dx.latitude);
    const double delta = std::remainder(rad(dx.longitude - de.longitude), 2 * pi);
    const double east = std::cos(other_lat) * std::sin(delta);
    const double north = std::cos(lat) * std::sin(other_lat)
        - std::sin(lat) * std::cos(other_lat) * std::cos(delta);
    const double dot = std::sin(lat) * std::sin(other_lat)
        + std::cos(lat) * std::cos(other_lat) * std::cos(delta);
    const double sine = std::hypot(east, north);
    const double angle = std::atan2(sine, dot);
    // Local north/east basis avoids subtracting nearly equal endpoint vectors.
    const double ex = -std::sin(lon), ey = std::cos(lon);
    const double nx = -std::sin(lat) * std::cos(lon);
    const double ny = -std::sin(lat) * std::sin(lon), nz = std::cos(lat);
    const double tx = (east * ex + north * nx) / sine;
    const double ty = (east * ey + north * ny) / sine;
    const double tz = north * nz / sine;
    const double ax = std::cos(lat) * std::cos(lon);
    const double ay = std::cos(lat) * std::sin(lon), az = std::sin(lat);
    std::vector<Location> points;
    points.reserve(static_cast<std::size_t>(subdivisions) + 1);
    points.push_back(de);
    for (unsigned i = 1; i < subdivisions; ++i) {
        const double step = angle * static_cast<double>(i) / subdivisions;
        const double c = std::cos(step), s = std::sin(step);
        const double x = c * ax + s * tx, y = c * ay + s * ty, z = c * az + s * tz;
        points.push_back({std::atan2(z, std::hypot(x,y)) * 180 / pi,
                          std::atan2(y,x) * 180 / pi});
    }
    points.push_back(dx);
    return points;
}
}
