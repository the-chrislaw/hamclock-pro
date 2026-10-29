// Great-circle path, Maidenhead and the land/border tables through the C ABI; the shared
// src/domain/geometry and src/map tables are the oracle.
#include "../hcpcore.h"
#include "domain/geometry.hpp"
#include "map/borders.hpp"
#include "map/land.hpp"
#include "test_util.hpp"
#include <string>

using namespace hcptest;

namespace {
void path() {
    HcpPath p{};
    // London -> New York: about 5570 km, initial bearing about 288 deg.
    check(hcp_path(51.5074, -0.1278, 40.7128, -74.0060, &p) == 1 && p.defined, "London-New York defined");
    check(near(p.distance_km, 5570, 15) && near(p.short_bearing_deg, 288.3, 1.0), "short path distance and bearing");
    check(near(p.distance_km + p.long_distance_km, 2 * 3.14159265358979 * 6371.0088, 1e-6) &&
              near(p.long_bearing_deg, std::fmod(p.short_bearing_deg + 180, 360), 1e-9), "long path is the complement");
    const auto g = hamclock::geometry({51.5074, -0.1278}, {40.7128, -74.0060});
    check(p.distance_km == g.distance_km && p.short_bearing_deg == g.short_bearing_deg, "same as the shared geometry");
    check(hcp_path(10, 20, 10, 20, &p) == 1 && !p.defined && p.distance_km == 0 && std::isnan(p.short_bearing_deg), "coincident");
    check(hcp_path(10, 20, -10, -160, &p) == 1 && !p.defined, "antipodal");
    check(!hcp_path(91, 0, 0, 0, &p) && !hcp_path(0, NAN, 0, 0, &p) && !hcp_path(0, 0, 0, 0, nullptr), "invalid coords");
}

void locator() {
    char buf[8];
    check(hcp_maidenhead(51.5074, -0.1278, 6, buf, sizeof buf) && std::string(buf) == "IO91wm", "London IO91wm");
    check(hcp_maidenhead(51.5074, -0.1278, 4, buf, sizeof buf) && std::string(buf) == "IO91", "4 characters");
    check(hcp_maidenhead(-90, -180, 6, buf, sizeof buf) && std::string(buf) == "AA00aa", "southwest corner");
    check(hcp_maidenhead(45.52, -122.67, 6, buf, sizeof buf) && std::string(buf) == hamclock::maidenhead({45.52, -122.67}, 6),
          "same as the shared encoder");
    check(!hcp_maidenhead(0, 0, 5, buf, sizeof buf) && !hcp_maidenhead(95, 0, 6, buf, sizeof buf) &&
              !hcp_maidenhead(0, 0, 6, buf, 0), "invalid precision / coords / buffer");
}

void tables() {
    const int rings = hcp_land_ring_count();
    size_t npts = 0;
    const float* pts = hcp_land_points(&npts);
    check(rings == int(sizeof hamclock::land_rings / sizeof hamclock::land_rings[0]) &&
              npts == sizeof hamclock::land_points / sizeof hamclock::land_points[0] && pts, "land tables exposed");
    bool ok = true;
    for (int i = 0; i < rings; ++i) {
        uint32_t s = 0, c = 0;
        ok = ok && hcp_land_ring(i, &s, &c) && c >= 3 && s + c <= npts && s == hamclock::land_rings[i].start;
    }
    check(ok, "every land ring in range");
    check(pts[0] == float(hamclock::land_points[0].longitude) && pts[1] == float(hamclock::land_points[0].latitude) &&
              pts[2 * npts - 1] == float(hamclock::land_points[npts - 1].latitude), "land points interleaved lon,lat");
    check(hcp_land_points(nullptr) == pts, "converted once");
    uint32_t s = 0, c = 0;
    check(!hcp_land_ring(rings, &s, &c) && !hcp_land_ring(-1, &s, &c) && !hcp_land_ring(0, nullptr, &c), "land index checked");

    const int parts = hcp_border_part_count();
    size_t nb = 0;
    const float* bp = hcp_border_points(&nb);
    check(parts == int(hamclock::border_part_count) && nb == hamclock::border_point_count && bp && parts > 0, "border tables exposed");
    ok = true;
    for (int i = 0; i < parts; ++i) ok = ok && hcp_border_part(i, &s, &c) && c >= 2 && s + c <= nb;
    check(ok, "every border part in range");
    bool in_range = true;
    for (size_t i = 0; i < nb; ++i) in_range = in_range && std::fabs(bp[2 * i]) <= 180 && std::fabs(bp[2 * i + 1]) <= 90;
    check(in_range, "border coordinates are lon/lat");
    check(!hcp_border_part(parts, &s, &c) && !hcp_border_part(-1, &s, &c), "border index checked");
}
} // namespace

int main() {
    path();
    locator();
    tables();
    return finish("geo");
}
