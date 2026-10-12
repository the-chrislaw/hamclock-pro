#include "projection.hpp"
#include <cmath>
#include <stdexcept>
namespace hamclock {
MapPoint map_project_equirectangular(Location location) {
    if (!valid_location(location)) throw std::invalid_argument("invalid map coordinate");
    const double lon = location.longitude == 180.0 ? -180.0 : location.longitude;
    const double x = (lon + 180.0) / 360.0;
    return {x >= 1.0 ? std::nextafter(1.0, 0.0) : x, (90.0 - location.latitude) / 180.0};
}
Location map_unproject_equirectangular(MapPoint point) {
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || point.x < 0 || point.x > 1 || point.y < 0 || point.y > 1)
        throw std::invalid_argument("invalid normalized map coordinate");
    return {90.0 - point.y * 180.0, point.x == 1.0 ? -180.0 : point.x * 360.0 - 180.0};
}
MapSegments map_split_equirectangular(Location first, Location second) {
    const auto a = map_project_equirectangular(first), b = map_project_equirectangular(second);
    MapSegments result;
    double dx = b.x - a.x;
    if (std::abs(dx) <= 0.5) {
        result.segments[0] = {a, b}; result.count = 1; return result;
    }
    const bool east = dx < 0;
    const double unwrapped = b.x + (east ? 1.0 : -1.0);
    const double edge = east ? 1.0 : 0.0;
    const double fraction = (edge - a.x) / (unwrapped - a.x);
    const double y = a.y + fraction * (b.y - a.y);
    result.segments[0] = {a, {edge, y}};
    result.segments[1] = {{east ? 0.0 : 1.0, y}, b};
    result.count = 2;
    return result;
}
}
