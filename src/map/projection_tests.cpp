#include "projection.hpp"
#include "borders.hpp"
#include "coastline.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace hamclock {
int run_map_tests() {
    int failures = 0;
    auto check = [&](bool pass, const char* name) { if (!pass) { ++failures; std::cerr << "Map test failed: " << name << '\n'; } };
    auto near = [](double a, double b) { return std::abs(a-b) < 1e-12; };
    const auto origin = map_project_equirectangular({0,0});
    check(near(origin.x,.5) && near(origin.y,.5), "origin");
    check(near(map_project_equirectangular({90,-180}).y,0), "north pole");
    check(near(map_project_equirectangular({-90,180}).x,0) && near(map_project_equirectangular({-90,180}).y,1), "south pole and seam");
    for (const Location p : {Location{37.7749,-122.4194},Location{-89,179.99},Location{90,0},Location{0,-180}}) {
        const auto result = map_unproject_equirectangular(map_project_equirectangular(p));
        check(near(result.latitude,p.latitude) && near(result.longitude,p.longitude), "round trip");
    }
    check(map_project_equirectangular({0,std::nextafter(180.0,0.0)}).x < 1.0, "exclusive east endpoint");
    check(map_unproject_equirectangular({1,.5}).longitude == -180, "inverse right edge");
    const auto east = map_split_equirectangular({10,170},{30,-170});
    check(east.count == 2 && near(east.segments[0].second.x,1) && near(east.segments[1].first.x,0) && near(east.segments[0].second.y,70.0/180), "east dateline");
    const auto west = map_split_equirectangular({30,-170},{10,170});
    check(west.count == 2 && near(west.segments[0].second.x,0) && near(west.segments[1].first.x,1) && near(west.segments[0].second.y,70.0/180), "west dateline");
    check(map_split_equirectangular({0,-10},{10,10}).count == 1, "normal segment");
    const auto seam = map_split_equirectangular({0,180},{10,170});
    check(seam.count == 2 && std::isfinite(seam.segments[0].second.y), "endpoint on seam");
    for (const Location p : {Location{91,0}, Location{0,181}, Location{std::numeric_limits<double>::quiet_NaN(),0}, Location{0,std::numeric_limits<double>::infinity()}}) {
        bool rejected = false; try { (void)map_project_equirectangular(p); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "invalid geographic input");
    }
    for (const MapPoint p : {MapPoint{-0.01,0},MapPoint{0,1.01},MapPoint{std::numeric_limits<double>::quiet_NaN(),0}}) {
        bool rejected = false; try { (void)map_unproject_equirectangular(p); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "invalid normalized input");
    }
    check(coastline_point_count > 1000 && coastline_point_count <= 100000 && coastline_part_count <= 10000, "coastline counts");
    std::size_t next = 0;
    for (const auto part : coastline_parts) {
        check(part.start == next && part.count >= 2 && part.start <= coastline_point_count && part.count <= coastline_point_count - part.start, "coastline part bounds");
        next = static_cast<std::size_t>(part.start) + part.count;
    }
    check(next == coastline_point_count, "coastline coverage");
    for (const auto p : coastline_points) check(valid_location({p.latitude,p.longitude}), "coastline coordinate");
    { // Natural Earth 110m boundary lines: bounds, coverage, and the 49th parallel is there.
      check(border_point_count > 1000 && border_point_count <= 100000 && border_part_count <= 10000, "border counts");
      std::size_t bn = 0; bool us_ca = false;
      for (const auto part : border_parts) {
          check(part.start == bn && part.count >= 2 && part.count <= border_point_count - part.start, "border part bounds");
          bn = static_cast<std::size_t>(part.start) + part.count;
      }
      check(bn == border_point_count, "border coverage");
      for (const auto p : border_points) {
          check(valid_location({p.latitude, p.longitude}), "border coordinate");
          if (std::fabs(p.latitude - 49.0) < 0.2 && p.longitude > -110 && p.longitude < -100) us_ca = true;
      }
      check(us_ca, "US-Canada border near 49N"); }
    // Azimuthal equidistant: centre, cardinal directions, antipode, round trips.
    {
        auto close = [](double a, double b, double e = 1e-9) { return std::abs(a - b) < e; };
        const Location de{40.0, -105.0};
        const auto c0 = map_project_azimuthal(de, de);
        check(close(c0.x, 0) && close(c0.y, 0), "azimuthal centre");
        const auto n = map_project_azimuthal({0, 0}, {90, 0});
        check(close(n.x, 0) && close(n.y, 0.5), "azimuthal north pole is half way up");
        const auto e = map_project_azimuthal({0, 0}, {0, 90});
        check(close(e.x, 0.5) && close(e.y, 0), "azimuthal east 90");
        const auto s = map_project_azimuthal({0, 0}, {-45, 0});
        check(close(s.x, 0) && close(s.y, -0.25), "azimuthal south 45");
        check(close(std::hypot(map_project_azimuthal(de, {-40, 75}).x, map_project_azimuthal(de, {-40, 75}).y), 1.0, 1e-9), "azimuthal antipode on rim");
        // distance from the centre is the great-circle angle: Denver to London is ~69 degrees (7,600 km)
        const auto lo = map_project_azimuthal(de, {51.5, -0.1});
        check(close(std::hypot(lo.x, lo.y) * 180.0, 69.0, 3.0) , "azimuthal Denver-London distance");
        for (const Location p : {Location{51.5, -0.1}, Location{-33.9, 151.2}, Location{85, 10}, Location{-60, -170}, Location{40, 10}}) {
            const auto r = map_unproject_azimuthal(de, map_project_azimuthal(de, p));
            check(close(r.latitude, p.latitude, 1e-7) && close(std::remainder(r.longitude - p.longitude, 360.0), 0, 1e-7), "azimuthal round trip");
        }
        bool rejected = false; try { (void)map_unproject_azimuthal(de, {0.9, 0.9}); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "azimuthal outside disc rejected");
    }
    // Robinson: table knots, symmetry, round trips, outline.
    {
        auto close = [](double a, double b, double e = 1e-9) { return std::abs(a - b) < e; };
        check(close(map_project_robinson({0, 0}).x, 0.5) && close(map_project_robinson({0, 0}).y, 0.5), "robinson origin");
        check(close(map_project_robinson({90, 0}).y, 0.0) && close(map_project_robinson({-90, 0}).y, 1.0), "robinson poles");
        check(close(map_project_robinson({90, 180}).x, 0.5 + 0.5322 / 2), "robinson pole line is 0.5322 of the equator");
        check(close(map_project_robinson({0, 180}).x, 1.0) && close(map_project_robinson({0, -180}).x, 0.0), "robinson equator edges");
        check(close(map_project_robinson({45, 0}).y, 0.5 - 0.5571 / 2), "robinson 45 N knot");
        check(close(map_project_robinson({-45, 20}).y, 1.0 - map_project_robinson({45, 20}).y), "robinson north/south symmetry");
        for (const Location p : {Location{37.7, -122.4}, Location{-33.9, 151.2}, Location{72.3, 100}, Location{-1.5, -179}, Location{0, 0}}) {
            const auto r = map_unproject_robinson(map_project_robinson(p));
            check(close(r.latitude, p.latitude, 1e-9) && close(r.longitude, p.longitude, 1e-9), "robinson round trip");
        }
        bool rejected = false; try { (void)map_unproject_robinson({0.0, 0.0}); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "robinson outside outline rejected");
    }
    {   // MapFrame: one transform for pixels <-> locations (G4)
        auto close = [](double a, double b, double tol = 1e-6) { return std::abs(a - b) < tol; };
        for (const auto& size : {std::pair<double, double>{1024, 600}, {800, 480}, {1920, 1080}, {300, 900}}) {
            const auto az = MapFrame::fit(MapProjection::Azimuthal, {39.7, -105}, size.first, size.second);
            const auto rb = MapFrame::fit(MapProjection::Robinson, {0, 30}, size.first, size.second);
            check(az.w == az.h && az.x >= 0 && az.y >= 0 && az.x + az.w <= size.first + 1e-9 && az.y + az.h <= size.second + 1e-9, "azimuthal frame fits");
            check(rb.x >= 0 && rb.y >= 0 && rb.x + rb.w <= size.first + 1e-9 && rb.y + rb.h <= size.second + 1e-9 && close(rb.h / rb.w, MapFrame::robinson_aspect), "robinson frame fits");
            const auto c = az.to_pixel({39.7, -105});
            check(c && close(c->x, az.x + az.w / 2) && close(c->y, az.y + az.h / 2), "azimuthal centre is the frame centre");
            const auto north = az.to_pixel({60, -105});
            check(north && close(north->x, c->x) && north->y < c->y, "azimuthal north is up");
            for (const Location p : {Location{51.5, -0.1}, Location{-33.9, 151.2}, Location{0, 179}, Location{-70, -20}}) {
                for (const auto* f : {&az, &rb}) {
                    const auto px = f->to_pixel(p);
                    const auto back = px ? f->to_location(px->x, px->y) : std::nullopt;
                    check(back && close(back->latitude, p.latitude, 1e-6) && close(std::remainder(back->longitude - p.longitude, 360.0), 0.0, 1e-6), "frame round trip");
                }
            }
            check(!az.to_location(az.x + 1, az.y + 1), "azimuthal corner is outside the disc");
            check(!rb.to_location(rb.x + 1, rb.y + 1), "robinson corner is outside the outline");
            const auto mid = rb.to_pixel({0, 30});
            check(mid && close(mid->x, rb.x + rb.w / 2) && close(mid->y, rb.y + rb.h / 2), "robinson centre longitude is the middle");
        }
        check(!MapFrame::fit(MapProjection::Equirectangular, {0, 0}, 100, 100).to_pixel({0, 0}), "flat map uses MapViewport");
        check(!MapFrame::fit(MapProjection::Azimuthal, {0, 0}, 0, 100).to_pixel({0, 0}), "empty item has no frame");
    }
    return failures;
}
}
