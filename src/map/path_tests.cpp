#include "path.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace hamclock {
int run_path_tests() {
    int failures = 0;
    auto check = [&](bool pass, const char* name) { if (!pass) { ++failures; std::cerr << "Path test failed: " << name << '\n'; } };
    auto near = [](double a, double b) { return std::abs(a-b) < 1e-8; };
    const auto eq = map_short_path({0,0},{0,90},4);
    check(eq.size()==5 && near(eq[2].latitude,0) && near(eq[2].longitude,45), "known equator midpoint");
    const auto seam = map_short_path({0,170},{0,-170},2);
    check(seam.size()==3 && near(std::abs(seam[1].longitude),180), "dateline shortest path");
    const auto polar = map_short_path({80,0},{80,180},2);
    check(near(polar[1].latitude,90), "path crossing north pole");
    const auto south = map_short_path({-80,0},{-80,180},2);
    check(near(south[1].latitude,-90), "path crossing south pole");
    const auto same = map_short_path({90,20},{90,140});
    check(same.size()==1 && same[0].longitude==20, "coincident pole preserves DE");
    for (unsigned n : {1u,64u,512u}) {
        const Location de{37.7749,-122.4194}, dx{51.5074,-0.1278};
        const auto points=map_short_path(de,dx,n);
        check(points.size()==n+1 && points.front().latitude==de.latitude && points.front().longitude==de.longitude && points.back().latitude==dx.latitude && points.back().longitude==dx.longitude, "count and exact endpoints");
        const double distance=geometry(de,dx).distance_km;
        for (std::size_t i=0;i<points.size();++i) {
            check(valid_location(points[i]), "valid samples");
            check(std::abs(geometry(de,points[i]).distance_km-distance*static_cast<double>(i)/n)<1e-7, "equal arc spacing");
        }
    }
    for (const auto dx : {Location{0,180},Location{0,180-1e-11},Location{91,0},Location{0,181},Location{0,std::numeric_limits<double>::infinity()},Location{std::numeric_limits<double>::quiet_NaN(),0}}) {
        bool rejected=false;try {(void)map_short_path({0,0},dx);} catch(const std::invalid_argument&) {rejected=true;}check(rejected,"undefined or invalid destination");
    }
    for (unsigned n : {0u,513u,std::numeric_limits<unsigned>::max()}) {
        bool rejected=false;try {(void)map_short_path({0,0},{0,90},n);} catch(const std::invalid_argument&) {rejected=true;}check(rejected,"subdivision bounds");
    }
    // Outside the degeneracy tolerance, near-antipodes must remain finite.
    const auto almost = map_short_path({0,0},{0,179.999999},8);
    for (const auto point : almost) check(valid_location(point), "defined near antipode");
    bool bad_de=false;try {(void)map_short_path({91,0},{0,0});}catch(const std::invalid_argument&) {bad_de=true;}check(bad_de,"invalid DE");
    // Long path: complementary arc with exact endpoints and equal spacing.
    for (const auto& pair : {std::pair<Location,Location>{{37.7749,-122.4194},{51.5074,-0.1278}},
                             std::pair<Location,Location>{{0,0},{0,90}}, std::pair<Location,Location>{{0,170},{0,-170}}}) {
        const unsigned n = 64;
        const auto lp = map_long_path(pair.first, pair.second, n);
        const auto g = geometry(pair.first, pair.second);
        check(lp.size()==n+1 && lp.front().longitude==pair.first.longitude && lp.back().latitude==pair.second.latitude, "long path endpoints");
        double sum = 0;
        for (std::size_t i = 1; i < lp.size(); ++i) sum += geometry(lp[i-1], lp[i]).distance_km;
        check(std::abs(sum - g.long_distance_km) < 1e-3 * g.long_distance_km, "long path length is the long arc");
    }
    const auto lq = map_long_path({0,0},{0,90},4);
    check(near(lq[1].longitude,-67.5) && near(lq[1].latitude,0), "long path equator heads west");
    check(map_long_path({90,20},{90,140}).size()==1, "long path coincident");
    bool lrej=false;try {(void)map_long_path({0,0},{0,180});}catch(const std::invalid_argument&) {lrej=true;}check(lrej,"long path antipodal rejected");
    return failures;
}
}
