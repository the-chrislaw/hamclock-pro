#include "ui/banner_text.hpp"

#include <cstdio>

using namespace hamclock;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

int main() {
    CHECK(banner_text(0, " w1aw ") == "W1AW");
    CHECK(banner_text(1, "W1AW") == "ON AIR");
    CHECK(banner_text(2, "w1aw") == "73 DE W1AW");
    CHECK(banner_text(2, "") == "73");
    CHECK(banner_text(3, "K1") == "K1");
    CHECK(banner_text(-1, "K1") == "73 DE K1");
    CHECK(next_banner_mode(0) == 1 && next_banner_mode(2) == 0);
    return failures ? 1 : 0;
}
