// Space weather impact port; mirrors src/widgets/solarimpact/tests/solar_impact_regression.cpp (the
// oracle) on its fixtures (noaa-scales.json, kyoto-dst.json) and synthetic storms, plus the C ABI.
#include "../hcpcore.h"
#include "../impact.hpp"
#include "test_util.hpp"
#include <cstring>
#include <string>
#include <vector>

using namespace hcptest;
using namespace hcp::impact;

namespace {
constexpr int64_t kNowMs = 1791945000000LL; // 2026-10-14 02:30 UTC, as the Qt test
constexpr int64_t kHourMs = 3600 * 1000LL;

std::vector<hcp::dst::Sample> dst_of(const std::string& body) { return hcp::dst::parse(body).value_or(std::vector<hcp::dst::Sample>{}); }
} // namespace

int main() {
    check(effect('G', 3).find("50 deg") != std::string::npos && effect('R', 0) == "No HF impact" && effect('X', 1).empty() &&
              effect('S', 6).empty() && effect('S', -1).empty(),
          "effect text and bounds");
    const std::vector<hcp::dst::Sample> none;
    check(ranked(hcp::scales::Scales{}, none, kNowMs).empty() && headline(hcp::scales::Scales{}, none, kNowMs).empty(),
          "nothing before data");

    const auto quiet = hcp::scales::parse(fixture("noaa-scales.json"));
    check(quiet.valid && headline(quiet, none, kNowMs) == "All quiet", "quiet fixture");
    auto rows = ranked(quiet, none, kNowMs);
    check(rows.size() == 3 && rows[0].letter == 'R' && rows[2].letter == 'G', "ties keep R, S, G order");

    // Synthetic storm (as the Qt test): G4 now, R2 now, S0 now but 24 h max S1.
    const auto storm = hcp::scales::parse(
        R"({"0":{"DateStamp":"2026-10-14","TimeStamp":"03:00:00","R":{"Scale":"2"},"S":{"Scale":"0"},"G":{"Scale":"4"}},)"
        R"("-1":{"R":{"Scale":"1"},"S":{"Scale":"1"},"G":{"Scale":"3"}}})");
    rows = ranked(storm, none, kNowMs);
    check(rows.size() == 3 && rows[0].letter == 'G' && rows[1].letter == 'R' && rows[2].letter == 'S', "ranked G, R, S by current level");
    check(rows[0].word == "Severe" && rows[2].max24 == 1, "word and 24 h max");
    check(headline(storm, none, kNowMs).rfind("G4 Severe: Aurora to about 45 deg", 0) == 0, "headline names the worst");

    const auto tie = hcp::scales::parse(
        R"({"0":{"DateStamp":"2026-10-14","TimeStamp":"03:00:00","R":{"Scale":"0"},"S":{"Scale":"0"},"G":{"Scale":"0"}},)"
        R"("-1":{"R":{"Scale":"0"},"S":{"Scale":"2"},"G":{"Scale":"0"}}})");
    check(ranked(tie, none, kNowMs)[0].letter == 'S' && headline(tie, none, kNowMs) == "All quiet", "24 h max breaks ties");

    // Dst joins while fresh: the real fixture is quiet, then a synthetic intense storm, then stale.
    check(dst_level(-29) == 0 && dst_level(-30) == 1 && dst_level(-50) == 2 && dst_level(-100) == 3 && dst_level(-251) == 5,
          "dst levels");
    const auto g1 = hcp::scales::parse(
        R"({"0":{"DateStamp":"2026-10-14","TimeStamp":"03:00:00","R":{"Scale":"0"},"S":{"Scale":"0"},"G":{"Scale":"1"}},)"
        R"("-1":{"R":{"Scale":"0"},"S":{"Scale":"0"},"G":{"Scale":"1"}}})");
    const auto real_dst = dst_of(fixture("kyoto-dst.json"));
    check(!real_dst.empty(), "dst fixture present");
    // The Qt test's clock sits within 6 h of the fixture's last record; do the same here.
    const int64_t dst_now = real_dst.empty() ? kNowMs : real_dst.back().time_ms + kHourMs / 2;
    rows = ranked(g1, real_dst, dst_now);
    check(rows.size() == 4 && rows[0].letter == 'G' && rows[3].letter == 'D' && rows[3].word == "Quiet", "quiet Dst ranks last");
    const auto intense = dst_of(R"([{"time_tag":"2026-10-14T01:00:00","dst":-140},{"time_tag":"2026-10-14T02:00:00","dst":-120}])");
    rows = ranked(g1, intense, kNowMs);
    check(rows.size() == 4 && rows[0].letter == 'D' && rows[0].level == 3 && rows[0].word == "Intense" && rows[0].max24 == 3 &&
              headline(g1, intense, kNowMs).rfind("D3 Intense: Intense storm", 0) == 0,
          "intense Dst ranks first");
    check(ranked(g1, intense, kNowMs + 7 * kHourMs).size() == 3, "stale Dst is dropped from the ranking");

    // C ABI over the parsed products, as the TV feeds hold them.
    const std::string storm_body =
        R"({"0":{"DateStamp":"2026-10-14","TimeStamp":"03:00:00","R":{"Scale":"2"},"S":{"Scale":"0"},"G":{"Scale":"4"}},)"
        R"("-1":{"R":{"Scale":"1"},"S":{"Scale":"1"},"G":{"Scale":"3"}}})";
    HcpScales sc;
    check(hcp_scales_parse(storm_body.data(), storm_body.size(), &sc) == 1, "C ABI scales parse");
    const HcpDst dsts[] = {{kNowMs - 2 * kHourMs, -140}, {kNowMs - kHourMs, -120}};
    HcpImpactRow out[4];
    char head[128];
    const int n = hcp_impact_rank(&sc, dsts, 2, kNowMs, out, 4, head, sizeof head);
    check(n == 4 && out[0].letter == 'G' && out[0].level == 4 && out[1].letter == 'D' && std::strcmp(out[1].word, "Intense") == 0 &&
              std::string(out[0].effect) == effect('G', 4) && std::string(head).rfind("G4 Severe: ", 0) == 0,
          "C ABI ranks with Dst");
    check(hcp_impact_rank(&sc, nullptr, 0, kNowMs, out, 4, head, sizeof head) == 3, "C ABI without Dst");
    HcpScales invalid{};
    check(hcp_impact_rank(&invalid, dsts, 2, kNowMs, out, 4, head, sizeof head) == 0 && head[0] == 0 &&
              hcp_impact_rank(nullptr, dsts, 2, kNowMs, out, 4, head, sizeof head) == 0,
          "C ABI without scales");
    return finish("impact");
}
