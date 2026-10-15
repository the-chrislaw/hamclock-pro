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
    CHECK(banner_is_callsign(0) && banner_is_callsign(3) && !banner_is_callsign(1) && !banner_is_callsign(-1));
    CHECK(next_banner_mode(0) == 1 && next_banner_mode(2) == 0);
    CHECK(banner_color(0, 0).isEmpty() && banner_color(99, 0).isEmpty() && banner_color(-1, 0).isEmpty());
    CHECK(banner_color(1, 0) == "#ffb000" && banner_color(4, 5) == "#ffffff");
    CHECK(banner_color(5, 0) != banner_color(5, 6) && banner_color(5, 0) == banner_color(5, 12));
    CHECK(banner_color(5, -1).startsWith('#'));
    return failures ? 1 : 0;
}
