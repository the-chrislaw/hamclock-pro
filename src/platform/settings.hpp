#pragma once

#include "../domain/geometry.hpp"
#include <string>

namespace hamclock {
struct StationSettings {
    Location de;
    Location dx;
    std::string callsign;
    std::string appearance = "system";
    // Optional IANA zone id for DE local time (e.g. "America/New_York"); empty = none.
    // Shape-checked only (1-63 of [A-Za-z0-9_+-/]); the tz database decides validity.
    std::string de_zone;
};

// Explicit caller-selected local path. Invalid data throws invalid_argument;
// filesystem failures throw runtime_error. Explicit saves replace regular files;
// failed loads do not fall back to defaults or initiate an automatic save.
void save_settings(const std::string& path, const StationSettings& settings);
// Atomic create-only publication: fails if any final destination already exists.
void save_new_settings(const std::string& path, const StationSettings& settings);
StationSettings load_settings(const std::string& path);
int run_settings_tests();
} // namespace hamclock
