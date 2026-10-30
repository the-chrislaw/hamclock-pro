#pragma once
#include <optional>
namespace hamclock {
// Pure view maths for the equirectangular map: no Qt, so it is unit tested
// directly. The map fills its item instead of letterboxing: the scale is the
// smallest that covers the item, crops polar latitudes when the item is
// wider than 2:1 (never below +/-min_visible_latitude, wrapping longitude
// instead) and crops longitude when the item is taller than 2:1.
struct MapViewport {
    static constexpr double min_visible_latitude = 70.0;
    double width = 0, height = 0;   // item size in pixels
    double scale = 0;               // pixels per degree
    double center_lon = 0;          // degrees at the horizontal centre
    double center_lat = 0;          // degrees at the vertical centre

    // focus_min/max_lat: latitudes (for example DE and DX) to keep visible.
    // zoom (clamped to 1..4) multiplies the covering scale; pan_lon/pan_lat
    // (user pan, degrees) replace the preferred centre and the focus centring
    // and are clamped so the world still covers the item vertically.
    static constexpr double max_zoom = 4.0;
    static MapViewport fill(double width, double height, double preferred_center_lon,
                            std::optional<double> focus_min_lat = std::nullopt,
                            std::optional<double> focus_max_lat = std::nullopt,
                            double zoom = 1.0, std::optional<double> pan_lon = std::nullopt,
                            std::optional<double> pan_lat = std::nullopt);
    // Preferred centre at zoom 1: DE, or the midpoint of DE and DX (the shorter
    // way round) when the item crops longitude and both fit with a margin.
    static constexpr double auto_center_margin_deg = 10.0;
    static double auto_center_lon(double width, double height, double de_lon, std::optional<double> dx_lon);
    // True when the world is wider than the item, so dragging sideways reveals more.
    bool longitude_cropped() const { return world_width() > width + 1e-9; }
    // Inverse of x_of/y_of for a pixel inside the item; latitude is clamped to +/-90.
    struct LatLon { double lat, lon; };
    LatLon location_at(double px, double py) const;
    // Centre (lon, lat) that puts location `at` under pixel (px, py) at this scale.
    LatLon center_for_anchor(LatLon at, double px, double py) const {
        return {at.lat + (py - height / 2.0) / scale, at.lon - (px - width / 2.0) / scale};
    }
    double world_width() const { return 360.0 * scale; }
    double world_height() const { return 180.0 * scale; }
    // Pixel x of normalized map x=0 (the -180 meridian) in the central copy.
    double origin_x() const;
    // Pixel y of the north pole.
    double origin_y() const { return height / 2.0 - (90.0 - center_lat) * scale; }
    // Pixel position of a location in the copy of the world nearest the centre.
    double x_of(double lon) const;
    double y_of(double lat) const { return height / 2.0 - (lat - center_lat) * scale; }
    double visible_half_latitude() const { return scale > 0 ? height / (2.0 * scale) : 0; }
    // Range [first,last] of world copies (k*world_width offsets) that touch the item.
    void copy_range(int& first, int& last) const;
};
}
