// Kyoto Dst port; mirrors src/widgets/solardst/tests (the oracle) on the same fixture
// (a real NOAA SWPC products/kyoto-dst.json captured 2026-10-14).
#include "../dst.hpp"
#include "../hcpcore.h"
#include "test_util.hpp"
#include <string>
#include <vector>

using namespace hcptest;

int main() {
    using namespace hcp;
    const auto body = fixture("kyoto-dst.json");
    check(!body.empty(), "fixture present");

    const auto s = dst::parse(body);
    check(s && s->size() == 167, "fixture record count");
    check(s && s->front().time_ms == 1791345600000LL && s->front().dst == -22, "first record 2026-10-07 04:00Z");
    check(s && s->back().time_ms == 1791943200000LL && s->back().dst == -13, "last record 2026-10-14 02:00Z");

    const auto odd = dst::parse(R"([{"time_tag":"2026-10-14T02:00:00","dst":-5},{"time_tag":"2026-10-14T01:00:00","dst":null},)"
                                R"({"time_tag":"bad","dst":3},{"time_tag":"2026-10-14T00:00:00","dst":-7}])");
    check(odd && odd->size() == 2 && (*odd)[0].dst == -7 && (*odd)[1].dst == -5, "bad records skipped, sorted oldest first");
    check(!dst::parse("") && !dst::parse("<html>") && !dst::parse("{}"), "garbage rejected");
    check(dst::word(-29) == "Quiet" && dst::word(-30) == "Weak" && dst::word(-50) == "Moderate" &&
              dst::word(-100) == "Intense" && dst::word(-251) == "Super" && dst::word(20) == "Quiet",
          "storm words");

    // Through the C ABI used by Swift: most recent cap records, oldest first.
    std::vector<HcpDst> out(200);
    check(hcp_dst_parse(body.data(), body.size(), out.data(), out.size()) == 167, "C ABI count");
    check(out[166].dst == -13 && out[166].time_ms == 1791943200000LL, "C ABI last record");
    HcpDst few[2];
    check(hcp_dst_parse(body.data(), body.size(), few, 2) == 2 && few[1].dst == -13, "C ABI keeps the newest");
    check(hcp_dst_parse("{}", 2, few, 2) == -1 && hcp_dst_parse(nullptr, 0, few, 2) == -1, "C ABI rejects non-product");
    char buf[16];
    check(hcp_dst_word(-120, buf, sizeof buf) && std::string(buf) == "Intense", "C ABI word");
    return finish("dst");
}
