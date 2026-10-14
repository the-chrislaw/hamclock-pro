#include "ui/path_text.hpp"

#include <cstdio>

using hamclock::path_summary;
using hamclock::path_detail;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

int main() {
    CHECK(path_summary("0 KM", "BEARING UNDEFINED") == "Set a DX target in Settings");
    CHECK(path_summary("8123 KM", "45 DEG / TRUE").startsWith("8123 KM"));
    CHECK(path_summary("8123 KM", "45 DEG / TRUE").endsWith("45 DEG / TRUE"));
    const auto g = hamclock::geometry({0, 0}, {0, 90});
    CHECK(path_detail(g, false).startsWith("Short path"));
    CHECK(path_detail(g, false).contains("10008 KM") && path_detail(g, false).contains("90 DEG"));
    CHECK(path_detail(g, true).startsWith("Long path") && path_detail(g, true).contains("30023 KM"));
    CHECK(path_detail(g, true).contains("270 DEG"));
    CHECK(path_detail(hamclock::geometry({0, 0}, {0, 0}), true) == "Set a DX target in Settings");
    return failures ? 1 : 0;
}
