// Space alerts port; mirrors src/widgets/solaralerts/tests (the oracle) on the same fixture
// (a trimmed real NOAA SWPC products/alerts.json captured 2026-10-13).
#include "../alerts.hpp"
#include "../hcpcore.h"
#include "test_util.hpp"
#include <string>
#include <vector>

using namespace hcptest;

namespace {
// 2026-10-<d> <h>:<m>Z in epoch ms (2026-10-13 00:00Z = 1791849600000).
int64_t utc(int d, int h, int m = 0) {
    return 1791849600000LL + (d - 13) * 86400000LL + h * 3600000LL + m * 60000LL;
}
} // namespace

int main() {
    using namespace hcp;
    const auto body = fixture("alerts.json");
    check(!body.empty(), "fixture present");

    const auto parsed = alerts::parse(body);
    check(parsed && parsed->size() == 12, "all 12 messages parsed");
    const auto all = parsed ? *parsed : std::vector<alerts::Alert>{};
    bool sorted = all.size() == 12;
    for (size_t i = 1; sorted && i < all.size(); ++i) sorted = all[i - 1].issued_ms >= all[i].issued_ms;
    check(sorted, "newest first");
    if (all.size() == 12) {
        check(all[0].code == "K04W" && all[0].kind == "Extended warning" &&
                  all[0].headline == "Geomagnetic K-index of 4 expected" && all[0].until_ms == utc(13, 21),
              "extended warning: headline and new end time");
        check(all[1].kind == "Warning" && all[1].until_ms == utc(13, 15), "warning: Valid To");
        check(all[2].kind == "Watch" && all[2].scale == "G1" && all[2].until_ms == 0, "watch: scale from category");
        check(all[3].cancelled && all[3].kind == "Cancel", "cancel notice recognised");
        check(all[5].kind == "Alert" && all[5].scale == "G1" && all[5].issued_ms == utc(10, 21, 45), "alert: scale and issue time");
        check(all[6].scale == "G3" && alerts::level(all[6]) == 2, "G3 is strong");
        check(alerts::level(all[5]) == 1 && alerts::level(all[0]) == 1, "G1 and warnings are level 1");
    }

    const auto at15 = alerts::active(all, utc(13, 15));
    check(at15.size() == 2 && at15[0].code == "K04W" && at15[1].code == "A20F", "active set at 15:00");
    check(alerts::active(all, utc(13, 22)).empty(), "warning drops after its end time, watch after 48 h");
    check(alerts::active(all, utc(10, 22)).size() == 4, "during the Oct 10 storm: one row per code");
    check(alerts::active(all, utc(16, 0)).empty(), "watch expires after 48 h");

    check(!alerts::parse("") && !alerts::parse("<html>"), "non-product rejected");
    const auto junk = alerts::parse("[{\"product_id\":\"X\",\"message\":\"junk\"},5]");
    check(junk && junk->empty(), "garbage rows skipped");
    check(alerts::parse_swpc_time("2026 Oct 13 1431") == utc(13, 14, 31) &&
              alerts::parse_swpc_time("2026 Oct 3 1431") == utc(3, 14, 31) &&
              alerts::parse_swpc_time("2026 Foo 13 1431") == 0 && alerts::parse_swpc_time("2026 Oct 13 2561") == 0,
          "SWPC time format");

    // Through the C ABI used by Swift: parse once, filter by the clock later.
    std::vector<HcpAlert> raw(32), out(32);
    const int n = hcp_alerts_parse(body.data(), body.size(), raw.data(), raw.size());
    check(n == 12 && std::string(raw[0].code) == "K04W" && raw[0].level == 1, "C ABI parse");
    const int m = hcp_alerts_active(raw.data(), static_cast<size_t>(n > 0 ? n : 0), utc(13, 15), out.data(), out.size());
    check(m == 2 && std::string(out[1].code) == "A20F" && std::string(out[1].scale) == "G1" &&
              std::string(out[0].headline) == "Geomagnetic K-index of 4 expected",
          "C ABI active set");
    check(hcp_alerts_parse("{}", 2, raw.data(), raw.size()) == -1 && hcp_alerts_parse(nullptr, 0, raw.data(), 1) == -1,
          "C ABI rejects non-product");
    return finish("alerts");
}
