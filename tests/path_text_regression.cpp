#include <QStringList>
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
    // Plain-space form for the substring checks; the real text uses no-break spaces on line 2.
    auto plain = [](QString t) { return t.replace(QChar(0x00A0), ' '); };
    CHECK(plain(path_detail(g, false)).contains("10008 KM") && plain(path_detail(g, false)).contains("90° TRUE"));
    CHECK(plain(path_detail(g, true)).startsWith("Long path") && plain(path_detail(g, true)).contains("30023 KM"));
    CHECK(plain(path_detail(g, true)).contains("270° TRUE"));
    CHECK(plain(path_detail(g, false, true)).contains("6218 MI") && !plain(path_detail(g, false, true)).contains("KM"));
    CHECK(plain(path_detail(g, true, true)).contains("18655 MI"));
    // Issue #401: three lines, and lines 2 and 3 have no breakable space (no orphan separator or unit).
    const auto lines = path_detail(g, false).split('\n');
    CHECK(lines.size() == 3 && lines[0] == "Short path" && !lines[1].contains(' ') && !lines[2].contains(' ') && lines[2].endsWith("TRUE"));
    CHECK(path_detail(hamclock::geometry({0, 0}, {0, 0}), true) == "Set a DX target in Settings");
    return failures ? 1 : 0;
}
