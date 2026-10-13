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
                              std::optional<double> focus_min_lat, std::optional<double> focus_max_lat) {
    MapViewport v;
    v.width = std::max(0.0, width);
    v.height = std::max(0.0, height);
    if (v.width <= 0 || v.height <= 0) return v;
    const double by_width = v.width / 360.0, by_height = v.height / 180.0;
    v.scale = std::max(by_width, by_height);
    // Very wide items would crop below the latitude floor: keep it and let
    // the longitude wrap (the renderer draws neighbouring world copies).
    v.scale = std::min(v.scale, v.height / (2.0 * min_visible_latitude));
    const bool longitude_cropped = v.scale * 360.0 > v.width + 1e-9 && by_width < by_height;
    v.center_lon = longitude_cropped ? wrap180(preferred_center_lon) : 0.0;
    const double half = std::min(90.0, v.visible_half_latitude());
    if (focus_min_lat && focus_max_lat) {
        const double mid = (*focus_min_lat + *focus_max_lat) / 2.0;
        v.center_lat = std::clamp(mid, -(90.0 - half), 90.0 - half);
    }
    return v;
}
double MapViewport::origin_x() const { return width / 2.0 - (center_lon + 180.0) * scale; }
double MapViewport::x_of(double lon) const { return width / 2.0 + wrap180(lon - center_lon) * scale; }
void MapViewport::copy_range(int& first, int& last) const {
    const double w = world_width();
    if (w <= 0) { first = 0; last = -1; return; }
    const double o = origin_x();
    first = static_cast<int>(std::floor((0.0 - o) / w - 1.0)) + 1;
    last = static_cast<int>(std::ceil((width - o) / w)) - 1;
}
}
