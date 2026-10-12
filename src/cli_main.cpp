// HamClock Pro offline command-line tools: station calculations, explicit
// station-file save/load and the bundled domain self-tests. No display.
#include "app/commands.hpp"
#include "app/pane_controls.hpp"
#include "app/station_edit.hpp"
#include "app/station_view.hpp"
#include "domain/geometry.hpp"
#include "map/path.hpp"
#include "map/projection.hpp"
#include "platform/settings.hpp"
#include <cstdio>
#include <cstring>

namespace {
constexpr int kUsageError = 2;

int usage(const char* program) {
    std::fprintf(stderr,
        "Usage: %s --self-test | --calculate DE_LAT DE_LON DX_LAT DX_LON | "
        "--save-station PATH CALLSIGN DE_LAT DE_LON DX_LAT DX_LON system|light|dark | "
        "--load-station PATH\n",
        program);
    return kUsageError;
}

int self_test() {
    const bool passed = hamclock::run_domain_tests() == 0 && hamclock::run_command_tests() == 0 &&
        hamclock::run_settings_tests() == 0 && hamclock::run_map_tests() == 0 &&
        hamclock::run_station_view_tests() == 0 && hamclock::run_path_tests() == 0 &&
        hamclock::run_pane_control_tests() == 0 && hamclock::run_station_edit_tests() == 0;
    std::puts(passed ? "All self-tests passed" : "Self-tests failed");
    return passed ? 0 : 1;
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) return usage(argv[0]);
    if (std::strcmp(argv[1], "--calculate") == 0) return hamclock::calculate_command(argc, argv);
    if (std::strcmp(argv[1], "--save-station") == 0 || std::strcmp(argv[1], "--load-station") == 0)
        return hamclock::settings_command(argc, argv);
    if (argc == 2 && std::strcmp(argv[1], "--self-test") == 0) return self_test();
    return usage(argv[0]);
}
