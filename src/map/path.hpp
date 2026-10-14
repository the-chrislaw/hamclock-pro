#pragma once
#include "../domain/geometry.hpp"
#include <vector>
namespace hamclock {
// Original spherical short great-circle sampling. Subdivisions must be 1..512.
// Defined routes return subdivisions+1 points, preserving exact endpoints.
// Coincident endpoints return only DE. Antipodal/near-antipodal routes classified
// as undefined by geometry() throw std::invalid_argument. No route is invented.
// Output coordinates are degrees, longitude within [-180,180]. A renderer
// must seam-split consecutive points; these are geographic, not screen lines.
std::vector<Location> map_short_path(Location de, Location dx, unsigned subdivisions = 64);
// Long path: the complementary arc of the same great circle (2*pi - short arc),
// same contract and exact endpoints. Coincident endpoints return only DE.
std::vector<Location> map_long_path(Location de, Location dx, unsigned subdivisions = 64);
int run_path_tests();
}
