#pragma once
// Shared scaffolding for the tvOS core regressions (fixtures from the Qt widget tests).
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace hcptest {
inline int checks = 0, failures = 0;
inline void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
inline std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
inline std::string fixture(const char* name) { return read_file(std::string(HCP_FIXTURE_DIR) + "/" + name); }
// Fixture of another widget: src/widgets/<widget>/tests/fixtures/<name>.
inline std::string widget_fixture(const char* widget, const char* name) {
    return read_file(std::string(HCP_WIDGET_FIXTURES) + "/" + widget + "/tests/fixtures/" + name);
}
inline bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }
inline int finish(const char* name) {
    std::cout << name << ": " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
} // namespace hcptest
