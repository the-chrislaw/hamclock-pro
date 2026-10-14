#include "ui/path_text.hpp"

#include <cstdio>

using hamclock::path_summary;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

int main() {
    CHECK(path_summary("0 KM", "BEARING UNDEFINED") == "Set a DX target in Settings");
    CHECK(path_summary("8123 KM", "45 DEG / TRUE").startsWith("8123 KM"));
    CHECK(path_summary("8123 KM", "45 DEG / TRUE").endsWith("45 DEG / TRUE"));
    return failures ? 1 : 0;
}
