#include "projection.hpp"
#include <algorithm>
#include <array>
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
namespace {
constexpr double pi = 3.14159265358979323846;
double rad(double d) { return d * pi / 180.0; }
double deg(double r) { return r * 180.0 / pi; }
struct RobinsonRow { double length, distance; };
constexpr std::array<RobinsonRow, 19> robinson_rows{{
    {1.0000, 0.0000}, {0.9986, 0.0620}, {0.9954, 0.1240}, {0.9900, 0.1860}, {0.9822, 0.2480},
    {0.9730, 0.3100}, {0.9600, 0.3720}, {0.9427, 0.4340}, {0.9216, 0.4958}, {0.8962, 0.5571},
    {0.8679, 0.6176}, {0.8350, 0.6769}, {0.7986, 0.7346}, {0.7597, 0.7903}, {0.7186, 0.8435},
    {0.6732, 0.8936}, {0.6213, 0.9394}, {0.5722, 0.9761}, {0.5322, 1.0000}}};
RobinsonRow robinson_row(double abs_lat) {
    const double f = std::clamp(abs_lat, 0.0, 90.0) / 5.0;
    const std::size_t i = std::min<std::size_t>(static_cast<std::size_t>(f), 17);
    const double t = f - static_cast<double>(i);
    return {robinson_rows[i].length + t * (robinson_rows[i + 1].length - robinson_rows[i].length),
            robinson_rows[i].distance + t * (robinson_rows[i + 1].distance - robinson_rows[i].distance)};
}
}
MapPoint map_project_azimuthal(Location centre, Location location) {
    if (!valid_location(centre) || !valid_location(location)) throw std::invalid_argument("invalid map coordinate");
    const double p0 = rad(centre.latitude), p = rad(location.latitude), dl = rad(location.longitude - centre.longitude);
    const double cosc = std::clamp(std::sin(p0) * std::sin(p) + std::cos(p0) * std::cos(p) * std::cos(dl), -1.0, 1.0);
    const double c = std::acos(cosc);
    const double east = std::cos(p) * std::sin(dl);
    const double north = std::cos(p0) * std::sin(p) - std::sin(p0) * std::cos(p) * std::cos(dl);
    const double s = std::hypot(east, north);
    if (s < 1e-12) return cosc < 0 ? MapPoint{1.0, 0.0} : MapPoint{0.0, 0.0}; // antipode direction is undefined: put it east on the rim
    const double r = c / pi;
    return {r * east / s, r * north / s};
}
Location map_unproject_azimuthal(Location centre, MapPoint point) {
    if (!valid_location(centre) || !std::isfinite(point.x) || !std::isfinite(point.y))
        throw std::invalid_argument("invalid azimuthal coordinate");
    const double rho = std::hypot(point.x, point.y);
    if (rho > 1.0 + 1e-12) throw std::invalid_argument("point outside the azimuthal disc");
    if (rho < 1e-15) return centre;
    const double c = std::min(rho, 1.0) * pi, p0 = rad(centre.latitude);
    const double sinc = std::sin(c), cosc = std::cos(c);
    const double lat = std::asin(std::clamp(cosc * std::sin(p0) + point.y / rho * sinc * std::cos(p0), -1.0, 1.0));
    double lon = centre.longitude + deg(std::atan2(point.x / rho * sinc, cosc * std::cos(p0) - point.y / rho * sinc * std::sin(p0)));
    lon = std::fmod(lon + 540.0, 360.0) - 180.0;
    return {deg(lat), lon};
}
MapPoint map_project_robinson(Location location) {
    if (!valid_location(location)) throw std::invalid_argument("invalid map coordinate");
    const auto row = robinson_row(std::abs(location.latitude));
    const double sign = location.latitude < 0 ? -1.0 : 1.0;
    return {0.5 + row.length * location.longitude / 360.0, 0.5 - sign * row.distance / 2.0};
}
namespace {
bool robinson_inverse(MapPoint point, Location& out) {
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || point.y < 0 || point.y > 1) return false;
    const double d = std::abs(1.0 - 2.0 * point.y);
    std::size_t i = 0;
    while (i < 17 && robinson_rows[i + 1].distance < d) ++i;
    const double t = (d - robinson_rows[i].distance) / (robinson_rows[i + 1].distance - robinson_rows[i].distance);
    const double abs_lat = std::clamp((static_cast<double>(i) + t) * 5.0, 0.0, 90.0);
    const double lon = (point.x - 0.5) * 360.0 / robinson_row(abs_lat).length;
    if (std::abs(lon) > 180.0 + 1e-9) return false;
    out = {point.y > 0.5 ? -abs_lat : abs_lat, std::clamp(lon, -180.0, 180.0)};
    return true;
}
double wrap_lon(double lon) {
    lon = std::fmod(lon + 180.0, 360.0);
    if (lon < 0) lon += 360.0;
    return lon - 180.0;
}
}
Location map_unproject_robinson(MapPoint point) {
    Location out{};
    if (!robinson_inverse(point, out)) throw std::invalid_argument("invalid robinson coordinate");
    return out;
}
MapFrame MapFrame::fit(MapProjection kind, Location centre, double width, double height) {
    MapFrame f;
    f.kind = kind;
    f.centre = centre;
    if (kind == MapProjection::Equirectangular || width <= 0 || height <= 0) return f;
    if (kind == MapProjection::Azimuthal) {
        f.w = f.h = std::min(width, height);
    } else {
        f.w = std::min(width, height / robinson_aspect);
        f.h = f.w * robinson_aspect;
    }
    f.x = (width - f.w) / 2.0;
    f.y = (height - f.h) / 2.0;
    return f;
}
std::optional<MapPoint> MapFrame::to_pixel(Location location) const {
    if (w <= 0 || !valid_location(location)) return std::nullopt;
    if (kind == MapProjection::Azimuthal) {
        const auto p = map_project_azimuthal(centre, location);
        return MapPoint{x + w / 2.0 + p.x * w / 2.0, y + h / 2.0 - p.y * h / 2.0};
    }
    if (kind == MapProjection::Robinson) {
        const auto p = map_project_robinson({location.latitude, wrap_lon(location.longitude - centre.longitude)});
        return MapPoint{x + p.x * w, y + p.y * h};
    }
    return std::nullopt;
}
std::optional<Location> MapFrame::to_location(double px, double py) const {
    if (w <= 0) return std::nullopt;
    if (kind == MapProjection::Azimuthal) {
        const MapPoint p{(px - x - w / 2.0) / (w / 2.0), -(py - y - h / 2.0) / (h / 2.0)};
        if (std::hypot(p.x, p.y) > 1.0) return std::nullopt;
        return map_unproject_azimuthal(centre, p);
    }
    if (kind == MapProjection::Robinson) {
        Location l{};
        if (!robinson_inverse({(px - x) / w, (py - y) / h}, l)) return std::nullopt;
        l.longitude = wrap_lon(l.longitude + centre.longitude);
        return l;
    }
    return std::nullopt;
}
}
