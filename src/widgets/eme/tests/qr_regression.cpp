// QR wrapper: structure checks that need no decoder (size by version, finder
// and timing patterns, capacity limits).
#include "../../../domain/astro_qr.hpp"
#include <iostream>
#include <string>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool finder_at(const hamclock::QrMatrix& m, int x0, int y0) {
    for (int y = 0; y < 7; ++y)
        for (int x = 0; x < 7; ++x) {
            const bool ring = x == 0 || x == 6 || y == 0 || y == 6;
            const bool core = x >= 2 && x <= 4 && y >= 2 && y <= 4;
            if (m.at(x0 + x, y0 + y) != (ring || core)) return false;
        }
    return true;
}
} // namespace

int main() {
    using namespace hamclock;
    // Version 1 is 21x21; 14 bytes fit at medium ECC.
    const auto small = qr_encode("HELLO WORLD");
    check(small.size == 21, "short text is a version-1 symbol");
    check(finder_at(small, 0, 0), "top-left finder");
    check(finder_at(small, small.size - 7, 0), "top-right finder");
    check(finder_at(small, 0, small.size - 7), "bottom-left finder");
    bool timing = true;
    for (int i = 8; i < small.size - 8; ++i)
        timing = timing && small.at(i, 6) == (i % 2 == 0) && small.at(6, i) == (i % 2 == 0);
    check(timing, "alternating timing patterns");
    check(small.at(8, small.size - 8), "dark module");

    const auto url = qr_encode("https://example.org/eme");
    check(url.size > 21 && (url.size - 17) % 4 == 0, "URL needs a larger valid version");
    check(qr_encode("https://example.org/eme").dark == url.dark, "deterministic");
    check(!small.at(-1, 0) && !small.at(0, small.size), "out of range is light");

    const auto huge = qr_encode(std::string(5000, 'x'));
    check(huge.size == 0 && huge.dark.empty(), "too long text gives an empty matrix");

    if (failures) { std::cerr << failures << " of " << checks << " checks failed\n"; return 1; }
    std::cout << checks << " checks passed\n";
    return 0;
}
