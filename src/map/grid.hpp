#pragma once
// Map grid overlays for the equirectangular map (pure, no Qt): graticule lines
// as degree segments. Maidenhead draws the 18x18 field boundaries (20 x 10 deg).
#include <cmath>
#include <vector>
namespace hamclock {
enum class MapGrid { None = 0, Tropics = 1, LatLng = 2, Maidenhead = 3, AzimuthRings = 4 };
inline constexpr int map_grid_count = 5;
struct GridLine { double lat1, lon1, lat2, lon2; };
inline std::vector<GridLine> grid_lines(MapGrid g) {
    std::vector<GridLine> out;
    auto parallel = [&](double lat) { out.push_back({lat, -180, lat, 180}); };
    auto meridian = [&](double lon) { out.push_back({-90, lon, 90, lon}); };
    switch (g) {
    case MapGrid::None: case MapGrid::AzimuthRings: break; // rings are centred on DE: see ring_points
    case MapGrid::Tropics:
        for (double lat : {-66.5634, -23.4366, 0.0, 23.4366, 66.5634}) parallel(lat);
        break;
    case MapGrid::LatLng:
        for (int lat = -60; lat <= 60; lat += 30) parallel(lat);
        for (int lon = -180; lon <= 180; lon += 30) meridian(lon);
        break;
    case MapGrid::Maidenhead:
        for (int lat = -80; lat <= 80; lat += 10) parallel(lat);
        for (int lon = -180; lon <= 180; lon += 20) meridian(lon);
        break;
    }
    return out;
}
// Maidenhead square boundaries (2 deg x 1 deg), without the field boundaries already
// drawn by grid_lines(Maidenhead). Shown only when zoomed in (see maidenhead_squares_min_zoom).
inline constexpr double maidenhead_squares_min_zoom = 3.0;
inline std::vector<GridLine> maidenhead_square_lines() {
    std::vector<GridLine> out;
    for (int lat = -89; lat <= 89; ++lat)
        if (lat % 10 != 0) out.push_back({double(lat), -180, double(lat), 180});
    for (int lon = -178; lon <= 178; lon += 2)
        if ((lon + 180) % 20 != 0) out.push_back({-90, double(lon), 90, double(lon)});
    return out;
}
// Azimuth rings: great-circle distance rings (degrees of arc, 30 deg = 3336 km)
// around the DE point. Pure; the renderer seam-splits consecutive points.
inline constexpr double azimuth_ring_step_deg = 30.0;
struct RingPoint { double lat, lon; };
inline std::vector<RingPoint> ring_points(double lat_deg, double lon_deg, double distance_deg, int n = 120) {
    const double pi = 3.14159265358979323846, r = pi / 180.0;
    const double la = lat_deg * r, lo = lon_deg * r, d = distance_deg * r;
    std::vector<RingPoint> out;
    for (int i = 0; i <= n; ++i) {
        const double b = 2 * pi * i / n;
        const double s = std::sin(la) * std::cos(d) + std::cos(la) * std::sin(d) * std::cos(b);
        const double lat = std::asin(std::fmax(-1.0, std::fmin(1.0, s)));
        double lon = lo + std::atan2(std::sin(b) * std::sin(d) * std::cos(la), std::cos(d) - std::sin(la) * s);
        lon = std::remainder(lon, 2 * pi);
        out.push_back({lat / r, lon / r});
    }
    return out;
}
}
