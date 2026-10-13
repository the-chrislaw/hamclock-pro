#include "station_edit.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
namespace hamclock {
int run_station_edit_tests() {
    int failures = 0;
    auto check = [&](bool okay, const char* name) {
        if (!okay) { ++failures; std::cerr << "Station edit test failed: " << name << '\n'; }
    };
    auto rejected = [](const StationEdit& edit) {
        try { (void)edit.validated(); } catch (const std::invalid_argument&) { return true; }
        return false;
    };
    try {
        const StationSettings baseline{{std::numeric_limits<double>::min(), -122.41234567890123},
                                      {51.5, -0.1}, "N0CALL/P", "system", ""};
        StationEdit edit(baseline);
        const auto initial = edit.validated();
        check(!edit.dirty() && initial.de.latitude == baseline.de.latitude &&
              initial.de.longitude == baseline.de.longitude && initial.dx.latitude == baseline.dx.latitude &&
              initial.dx.longitude == baseline.dx.longitude, "exact baseline including minimum normal");
        {
            auto zoned = baseline; zoned.de_zone = "Europe/London";
            StationEdit zone_edit(zoned);
            check(zone_edit.set(StationField::Callsign, "N1TEST") &&
                  zone_edit.validated().de_zone == "Europe/London", "station edit preserves DE zone");
        }
        for (const char* bad : {"", "-", ".", "1e", "1e999", "inf", "NaN", " 1", "1 ", "0x1p2", "91", "1e-320"}) {
            check(edit.set(StationField::DeLatitude, bad) && edit.dirty() && rejected(edit), "invalid bounded draft retained");
            check(edit.get(StationField::DeLatitude) == bad, "validation does not mutate draft");
            edit.cancel();
            check(!edit.dirty() && edit.validated().de.latitude == baseline.de.latitude, "cancel restores baseline");
        }
        check(edit.set(StationField::DeLatitude, "+1e1") && edit.validated().de.latitude == 10, "decimal exponent accepted");
        check(edit.set(StationField::DeLongitude, "181") && rejected(edit), "longitude bounds");
        edit.cancel();
        check(edit.set(StationField::Callsign, "bad_") && rejected(edit), "callsign validation reused");
        edit.cancel();
        check(edit.set(StationField::Appearance, "auto") && rejected(edit), "appearance validation reused");
        edit.cancel();
        for (auto field : {StationField::Callsign, StationField::DeLatitude, StationField::DeLongitude,
                           StationField::DxLatitude, StationField::DxLongitude, StationField::Appearance}) {
            const auto old = edit.get(field);
            const std::size_t limit = field == StationField::Callsign ? 16 : field == StationField::Appearance ? 6 : 64;
            check(!edit.set(field, std::string(limit + 1, '1')) && edit.get(field) == old && !edit.dirty(), "overlimit leaves draft unchanged");
            check(edit.set(field, std::string(limit, '1')), "exact limit retained for editing");
            edit.cancel();
        }
        const auto bad_field = static_cast<StationField>(999);
        check(!edit.set(bad_field, "1") && !edit.dirty(), "invalid field set rejected");
        bool get_rejected = false;
        try { (void)edit.get(bad_field); } catch (const std::invalid_argument&) { get_rejected = true; }
        check(get_rejected && !edit.dirty(), "invalid field get rejected");
        check(edit.set(StationField::Callsign, "N1TEST") && edit.set(StationField::DxLatitude, "-45") &&
              edit.set(StationField::DxLongitude, "170") && edit.set(StationField::Appearance, "dark"), "multiple draft edits");
        const auto changed = edit.validated();
        check(changed.callsign == "N1TEST" && changed.dx.latitude == -45 && changed.dx.longitude == 170 &&
              changed.appearance == "dark" && baseline.callsign == "N0CALL/P", "validated result independent of baseline");
        edit.cancel();
        check(!edit.dirty() && edit.validated().callsign == baseline.callsign, "cancel after valid edits");
        auto invalid_base = baseline; invalid_base.de.latitude = std::numeric_limits<double>::infinity();
        bool constructor_rejected = false;
        try { StationEdit invalid(invalid_base); } catch (const std::invalid_argument&) { constructor_rejected = true; }
        check(constructor_rejected, "invalid baseline rejected");
    } catch (const std::exception& error) {
        ++failures; std::cerr << "Unexpected station edit test exception: " << error.what() << '\n';
    }
    return failures;
}
}
