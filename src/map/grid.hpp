#pragma once
// Map grid overlays for the equirectangular map (pure, no Qt): graticule lines
// as degree segments. Maidenhead draws the 18x18 field boundaries (20 x 10 deg).
#include <vector>
namespace hamclock {
enum class MapGrid { None = 0, Tropics = 1, LatLng = 2, Maidenhead = 3 };
inline constexpr int map_grid_count = 4;
struct GridLine { double lat1, lon1, lat2, lon2; };
inline std::vector<GridLine> grid_lines(MapGrid g) {
    std::vector<GridLine> out;
    auto parallel = [&](double lat) { out.push_back({lat, -180, lat, 180}); };
    auto meridian = [&](double lon) { out.push_back({-90, lon, 90, lon}); };
    switch (g) {
    case MapGrid::None: break;
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
}
