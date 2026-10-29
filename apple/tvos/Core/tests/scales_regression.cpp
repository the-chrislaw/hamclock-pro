// NOAA scales port; mirrors src/widgets/solarscales/tests (the oracle) on the same fixture.
#include "../hcpcore.h"
#include "../scales.hpp"
#include "test_util.hpp"
#include <string>

using namespace hcptest;

int main() {
    using namespace hcp;
    const auto body = fixture("noaa-scales.json");
    check(!body.empty(), "fixture present");
    const auto s = scales::parse(body);
    check(s.valid, "fixture parses");
    check(s.observed_ms == 1791943740000LL, "observed time is UTC (2026-10-14 02:09Z)");
    check(s.current.r == 0 && s.current.s == 0 && s.current.g == 0, "current all quiet");
    check(s.past24h.g == 0, "24 h max");
    check(s.forecast[0].r_minor == 10 && s.forecast[0].r_major == 1 && s.forecast[0].s_prob == 5, "day 1 probabilities");
    check(s.forecast[1].s_prob == 1 && s.forecast[2].g == 0, "later days");

    const auto active = scales::parse(
        R"({"0":{"DateStamp":"2026-10-14","TimeStamp":"03:00:00","R":{"Scale":"2","Text":"moderate"},"S":{"Scale":"1"},"G":{"Scale":"4"}},)"
        R"("2":{"R":{"Scale":null,"MinorProb":"40","MajorProb":"10"},"S":{"Scale":null,"Prob":"15"},"G":{"Scale":"3","Text":"strong"}}})");
    check(active.valid && active.current.r == 2 && active.current.s == 1 && active.current.g == 4, "active levels");
    check(active.forecast[1].g == 3 && active.forecast[1].r_minor == 40 && active.forecast[1].s_prob == 15, "forecast values");
    check(active.forecast[0].r == -1 && active.forecast[2].s_prob == -1, "absent records stay -1");
    check(scales::scale_word(4) == "Severe" && scales::scale_word(0).empty() && scales::scale_word(9).empty(), "scale words");

    check(!scales::parse("").valid && !scales::parse("<html>").valid && !scales::parse("[]").valid && !scales::parse("{}").valid &&
              !scales::parse(R"({"0":{"DateStamp":"2026-10-14","TimeStamp":"03:00:00","R":{"Scale":null}}})").valid &&
              !scales::parse(R"({"0":{"R":{"Scale":"9"},"S":{"Scale":"0"},"G":{"Scale":"0"}}})").valid,
          "garbage, missing current state and out-of-range levels rejected");

    // C ABI.
    HcpScales c{};
    check(hcp_scales_parse(body.data(), body.size(), &c) == 1 && c.valid && c.observed_ms == s.observed_ms, "C ABI parse");
    check(c.forecast[0].r_minor == 10 && c.forecast[0].r_major == 1 && c.forecast[0].s_prob == 5 && c.forecast[1].s_prob == 1 &&
              c.current.g == 0 && c.past24h.g == 0 && c.forecast[0].r == -1, "C ABI fields");
    check(hcp_scales_parse("{}", 2, &c) == 0 && !c.valid && !hcp_scales_parse(body.data(), body.size(), nullptr), "C ABI rejects");
    char buf[16];
    check(hcp_scale_word(5, buf, sizeof buf) == 1 && std::string(buf) == "Extreme", "C ABI scale word");
    check(hcp_scale_word(0, buf, sizeof buf) == 0 && buf[0] == '\0' && hcp_scale_word(6, buf, sizeof buf) == 0, "C ABI no word");
    return finish("scales");
}
