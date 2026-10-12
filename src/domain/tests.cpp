#include "geometry.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace hamclock {
int run_domain_tests() {
    int failures = 0;
    auto check = [&](bool condition, const char* message) {
        if (!condition) { ++failures; std::cerr << "Domain test failed: " << message << '\n'; }
    };
    auto close = [](double a, double b, double tolerance = 1e-7) {
        return std::abs(a - b) <= tolerance;
    };
    auto invalid = [&](auto action, const char* message) {
        bool rejected = false;
        try { action(); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, message);
    };
    const auto east = geometry({0, 0}, {0, 1});
    check(close(east.distance_km, 111.1950802335), "equatorial degree distance");
    check(close(east.short_bearing_deg, 90), "east azimuth");
    check(close(east.long_bearing_deg, 270), "complementary azimuth");
    check(close(east.distance_km + east.long_distance_km, 40030.2288840718), "complementary circumference");
    const auto crossing = geometry({0, 179}, {0, -179});
    check(close(crossing.distance_km, 222.390160467), "dateline short arc");
    check(close(crossing.short_bearing_deg, 90), "dateline east bearing");
    check(geometry({90, -20}, {90, 75}).status == BearingStatus::Coincident, "north pole longitude equivalence");
    check(close(geometry({0, 0}, {90, 30}).distance_km, 10007.5572210179), "quarter circumference to pole");
    const auto identical = geometry({51.5, -0.1}, {51.5, -0.1});
    check(identical.status == BearingStatus::Coincident && identical.distance_km == 0
          && std::isnan(identical.short_bearing_deg) && std::isnan(identical.long_bearing_deg), "identical bearings undefined");
    const auto antipodal = geometry({0, 0}, {0, 180});
    check(antipodal.status == BearingStatus::Antipodal && std::isnan(antipodal.short_bearing_deg)
          && close(antipodal.distance_km, antipodal.long_distance_km), "antipodal bearings undefined");
    check(geometry({0, -180}, {0, 180}).status == BearingStatus::Coincident, "longitude endpoint normalization");
    const auto near_anti = geometry({0, 0}, {0, 179.999999});
    check(near_anti.status == BearingStatus::Defined && close(near_anti.short_bearing_deg, 90)
          && near_anti.distance_km < antipodal.distance_km, "near antipode retains precision");
    check(maidenhead({51.5074, -0.1278}) == "IO91wm", "London locator");
    check(maidenhead({0, 0}) == "JJ00aa", "origin boundary locator");
    check(maidenhead({-90, -180}) == "AA00aa", "southwest endpoint");
    check(maidenhead({90, 180}) == "AR09ax", "northern endpoint and wrapped longitude");
    check(maidenhead({0, std::nextafter(180.0, 0.0)}) == "RJ90xa", "interior eastern endpoint rounding");
    check(maidenhead({51.5074, -0.1278}, 2) == "IO" && maidenhead({51.5074, -0.1278}, 4) == "IO91", "locator precision");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    check(!valid_location({nan, 0}) && !valid_location({0, inf}) && !valid_location({91, 0})
          && !valid_location({0, -181}), "invalid coordinates");
    invalid([&] { geometry({nan, 0}, {0, 0}); }, "geometry rejects NaN");
    invalid([&] { geometry({0, 0}, {0, inf}); }, "geometry rejects infinity");
    invalid([&] { maidenhead({91, 0}); }, "locator rejects latitude");
    invalid([&] { maidenhead({0, 0}, 5); }, "locator rejects precision");
    return failures;
}
} // namespace hamclock
