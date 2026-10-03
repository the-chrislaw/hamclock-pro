#include "commands.hpp"
#include "../domain/geometry.hpp"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace hamclock {
namespace {
double coordinate(const char* raw) {
    if (!raw) throw std::invalid_argument("Missing coordinate");
    std::size_t length = 0;
    while (length <= 64 && raw[length] != '\0') ++length;
    if (length == 0 || length > 64)
        throw std::invalid_argument("Coordinates must be decimal numbers of at most 64 characters");
    const std::string text(raw, length);
    if (
        text.find_first_not_of("0123456789+-.eE") != std::string::npos)
        throw std::invalid_argument("Coordinates must be decimal numbers of at most 64 characters");
    char* end = nullptr;
    errno = 0;
    const double value = std::strtod(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || errno == ERANGE || !std::isfinite(value))
        throw std::invalid_argument("Coordinate is not a finite decimal number");
    return value;
}
}

int calculate_command(int argc, char** argv) {
    if (argc != 6) {
        std::fputs("Usage: hamclock-pro --calculate DE_LAT DE_LON DX_LAT DX_LON\n"
                   "Use signed degrees: north/east positive, south/west negative.\n", stderr);
        return 2;
    }
    try {
        const Location de{coordinate(argv[2]), coordinate(argv[3])};
        const Location dx{coordinate(argv[4]), coordinate(argv[5])};
        const auto result = geometry(de, dx);
        const auto de_grid = maidenhead(de);
        const auto dx_grid = maidenhead(dx);
        std::printf("{\"de_grid\":\"%s\",\"dx_grid\":\"%s\",\"distance_km\":%.6f,"
                    "\"long_distance_km\":%.6f,", de_grid.c_str(), dx_grid.c_str(),
                    result.distance_km, result.long_distance_km);
        if (result.status == BearingStatus::Defined) {
            std::printf("\"initial_bearing_deg\":%.6f,\"long_initial_bearing_deg\":%.6f,"
                        "\"bearing_status\":\"defined\"}\n",
                        result.short_bearing_deg, result.long_bearing_deg);
        } else {
            std::printf("\"initial_bearing_deg\":null,\"long_initial_bearing_deg\":null,"
                        "\"bearing_status\":\"%s\"}\n",
                        result.status == BearingStatus::Coincident ? "coincident" : "antipodal");
        }
        return 0;
    } catch (const std::invalid_argument& error) {
        std::fprintf(stderr, "Invalid location: %s\n", error.what());
        return 2;
    }
}

int run_command_tests() {
    if (coordinate("-122.4") != -122.4 || coordinate("1e1") != 10.0) return 1;
    const char* invalid[] = {"", "nan", "inf", "0x1p2", "1;echo", "1 2", "1e9999", "--1", "."};
    for (const char* input : invalid) {
        try { (void)coordinate(input); return 1; }
        catch (const std::invalid_argument&) {}
    }
    try { const std::string huge(65, '1'); (void)coordinate(huge.c_str()); return 1; }
    catch (const std::invalid_argument&) {}
    std::puts("Coordinate input validation tests passed");
    return 0;
}
}
