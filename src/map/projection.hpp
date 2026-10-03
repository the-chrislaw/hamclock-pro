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
int run_map_tests();
}
