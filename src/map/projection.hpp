#pragma once
#include "../domain/geometry.hpp"
#include <array>
#include <cstddef>
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
int run_map_tests();
}
