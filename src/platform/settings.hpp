#pragma once

#include "../domain/geometry.hpp"
#include <string>

namespace hamclock {
struct StationSettings {
    Location de;
    Location dx;
    std::string callsign;
    std::string appearance = "system";
};

// Explicit caller-selected local path. Invalid data throws invalid_argument;
// filesystem failures throw runtime_error. Explicit saves replace regular files;
// failed loads do not fall back to defaults or initiate an automatic save.
void save_settings(const std::string& path, const StationSettings& settings);
StationSettings load_settings(const std::string& path);
int run_settings_tests();
} // namespace hamclock
