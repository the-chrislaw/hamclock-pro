#include "viewport.hpp"
#include <algorithm>
#include <cmath>
namespace hamclock {
namespace {
double wrap180(double lon) {
    lon = std::fmod(lon + 180.0, 360.0);
    if (lon < 0) lon += 360.0;
    return lon - 180.0;
}
}
MapViewport MapViewport::fill(double width, double height, double preferred_center_lon,
                              std::optional<double> focus_min_lat, std::optional<double> focus_max_lat,
                              double zoom, std::optional<double> pan_lon, std::optional<double> pan_lat) {
    MapViewport v;
    v.width = std::max(0.0, width);
    v.height = std::max(0.0, height);
    if (v.width <= 0 || v.height <= 0) return v;
    const double by_width = v.width / 360.0, by_height = v.height / 180.0;
    v.scale = std::max(by_width, by_height);
    // Very wide items would crop below the latitude floor: keep it and let
    // the longitude wrap (the renderer draws neighbouring world copies).
    v.scale = std::min(v.scale, v.height / (2.0 * min_visible_latitude));
    zoom = std::clamp(zoom, 1.0, max_zoom);
    v.scale *= zoom;
    const bool longitude_cropped = (v.scale * 360.0 > v.width + 1e-9 && by_width < by_height) || zoom > 1.0 || pan_lon.has_value();
    v.center_lon = longitude_cropped ? wrap180(pan_lon ? *pan_lon : preferred_center_lon) : 0.0;
    const double half = std::min(90.0, v.visible_half_latitude());
    if (pan_lat) {
        v.center_lat = std::clamp(*pan_lat, -(90.0 - half), 90.0 - half);
    } else if (focus_min_lat && focus_max_lat) {
        const double mid = (*focus_min_lat + *focus_max_lat) / 2.0;
        v.center_lat = std::clamp(mid, -(90.0 - half), 90.0 - half);
    }
    return v;
}
double MapViewport::auto_center_lon(double width, double height, double de_lon, std::optional<double> dx_lon) {
    const auto v = fill(width, height, de_lon);
    if (!dx_lon || !v.longitude_cropped()) return de_lon;
    const double span = wrap180(*dx_lon - de_lon);
    const double visible = v.width / v.scale;
    if (std::abs(span) > visible - 2.0 * auto_center_margin_deg) return de_lon;
    return wrap180(de_lon + span / 2.0);
}
double MapViewport::origin_x() const { return width / 2.0 - (center_lon + 180.0) * scale; }
double MapViewport::x_of(double lon) const { return width / 2.0 + wrap180(lon - center_lon) * scale; }
MapViewport::LatLon MapViewport::location_at(double px, double py) const {
    if (scale <= 0) return {0, 0};
    return {std::clamp(center_lat - (py - height / 2.0) / scale, -90.0, 90.0), wrap180(center_lon + (px - width / 2.0) / scale)};
}
void MapViewport::copy_range(int& first, int& last) const {
    const double w = world_width();
    if (w <= 0) { first = 0; last = -1; return; }
    const double o = origin_x();
    first = static_cast<int>(std::floor((0.0 - o) / w - 1.0)) + 1;
    last = static_cast<int>(std::ceil((width - o) / w)) - 1;
}
}
