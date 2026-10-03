#include "station_view.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

namespace hamclock {
int run_station_view_tests() {
    int failures = 0;
    auto check = [&](bool condition, const char* name) {
        if (!condition) { ++failures; std::cerr << "Station view test failed: " << name << '\n'; }
    };
    auto rejects = [](const StationSettings& settings) {
        try { (void)make_station_view(settings); }
        catch (const std::invalid_argument&) { return true; }
        return false;
    };
    try {
        const StationSettings east{{0, 0}, {0, 1}, "n0call/p", "system"};
        const auto view = make_station_view(east);
        check(view.callsign == "N0CALL/P" && view.de_grid == "JJ00AA" && view.dx_grid == "JJ00MA",
              "uppercase callsign and locators");
        check(view.distance_text == "111 KM" && view.bearing_text == "90 DEG / TRUE",
              "known equatorial east path");
        check(view.de.latitude == east.de.latitude && view.de.longitude == east.de.longitude &&
              view.dx.latitude == east.dx.latitude && view.dx.longitude == east.dx.longitude,
              "coordinates retained");
        check(east.callsign == "n0call/p", "source settings unchanged");
        auto north = east; north.dx = {1, 0};
        check(make_station_view(north).bearing_text == "0 DEG / TRUE", "true north");
        north.dx.longitude = -0.001;
        check(make_station_view(north).bearing_text == "0 DEG / TRUE", "rounding wraps north");
        auto same = east; same.dx = same.de;
        const auto same_view = make_station_view(same);
        check(same_view.distance_text == "0 KM" && same_view.bearing_text == "BEARING UNDEFINED",
              "coincident bearing undefined");
        same.dx = {0, 180};
        const auto antipodal_view = make_station_view(same);
        check(antipodal_view.distance_text == "20015 KM" &&
              antipodal_view.bearing_text == "BEARING UNDEFINED", "antipodal bearing undefined");
        auto dateline = east; dateline.de = {0, 179}; dateline.dx = {0, -179};
        check(make_station_view(dateline).distance_text == "222 KM" &&
              make_station_view(dateline).bearing_text == "90 DEG / TRUE", "dateline short path");
        auto boundary = east; boundary.de = {-90, -180}; boundary.dx = {90, 180};
        check(make_station_view(boundary).de_grid == "AA00AA" &&
              make_station_view(boundary).dx_grid == "AR09AX", "pole endpoint locators");
        for (const auto& appearance : {"light", "dark", "system"}) {
            auto setting = east; setting.appearance = appearance;
            check(make_station_view(setting).callsign == "N0CALL/P", "appearance accepted");
        }
        auto invalid = east; invalid.de.latitude = 91;
        check(rejects(invalid), "out of bounds latitude");
        invalid = east; invalid.dx.longitude = -181;
        check(rejects(invalid), "out of bounds longitude");
        invalid = east; invalid.dx.latitude = std::numeric_limits<double>::quiet_NaN();
        check(rejects(invalid), "nonfinite coordinate");
        invalid = east; invalid.de.longitude = std::numeric_limits<double>::denorm_min();
        check(rejects(invalid), "settings subnormal guard retained");
        invalid = east; invalid.callsign.clear();
        check(rejects(invalid), "empty callsign rejected");
        invalid.callsign = std::string(17, 'A');
        check(rejects(invalid), "long callsign rejected");
        invalid.callsign = "N0CALL\n";
        check(rejects(invalid), "control character rejected");
        invalid.callsign = "N0CALL_";
        check(rejects(invalid), "unsupported callsign punctuation rejected");
        invalid = east; invalid.appearance = "automatic";
        check(rejects(invalid), "unknown appearance rejected");
    } catch (const std::exception& error) {
        ++failures; std::cerr << "Unexpected station view test exception: " << error.what() << '\n';
    }
    return failures;
}
} // namespace hamclock
