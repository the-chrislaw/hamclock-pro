#pragma once
#include "../domain/geometry.hpp"
#include <array>
#include <cstddef>
#include <optional>
namespace hamclock {
struct MapPoint { double x, y; };
struct MapSegment { MapPoint first, second; };
struct MapSegments { std::array<MapSegment, 2> segments{}; std::size_t count = 0; };
// Plate-carree normalized coordinates: x [0,1), y [0,1]. North is y=0.
// +180 and -180 coincide at the left edge. Invalid locations throw.
MapPoint map_project_equirectangular(Location location);
// Inverse accepts the inclusive right edge and normalizes it to -180.
Location map_unproject_equirectangular(MapPoint point);
// For coastline segments, assume the shorter longitudinal step. Split at
// the antimeridian with interpolated latitude; never connect across the map.
// This is a coastline drawing operation, not a great-circle path calculation.
MapSegments map_split_equirectangular(Location first, Location second);
// Azimuthal equidistant centred on `centre` (G4). Output is the unit disc:
// distance from the centre is angular distance / 180 degrees (0..1, the antipode
// is on the rim), north is up at the centre. The inverse throws outside the disc.
MapPoint map_project_azimuthal(Location centre, Location location);
Location map_unproject_azimuthal(Location centre, MapPoint point);
// Robinson (tabulated, linear between the 5-degree rows). x in [0,1] across the
// parallel's own length (so the outline narrows toward the poles), y in [0,1],
// north at y=0. Plate height/width of the true projection is about 0.506.
MapPoint map_project_robinson(Location location);
Location map_unproject_robinson(MapPoint point);
// Which projection the map draws. Equirectangular is the scrolling/zooming
// MapViewport; the others are drawn whole, fitted and centred in the item.
enum class MapProjection { Equirectangular = 0, Azimuthal = 1, Robinson = 2 };
inline constexpr int map_projection_count = 3;
// Pixel frame of a whole-world projection inside an item (G4): the ONE
// transform the base map, overlays, markers and taps share. Azimuthal is
// centred on `centre` (a disc, square frame); Robinson shifts longitudes so
// centre.longitude is in the middle. to_pixel is empty for Equirectangular
// (use MapViewport) or an invalid location; to_location is empty outside the
// outline.
struct MapFrame {
    static constexpr double robinson_aspect = 0.5072; // height / width of the outline
    MapProjection kind = MapProjection::Equirectangular;
    Location centre{0, 0};
    double x = 0, y = 0, w = 0, h = 0; // frame box in item pixels
    static MapFrame fit(MapProjection kind, Location centre, double width, double height);
    std::optional<MapPoint> to_pixel(Location location) const;
    std::optional<Location> to_location(double px, double py) const;
};
int run_map_tests();
}
