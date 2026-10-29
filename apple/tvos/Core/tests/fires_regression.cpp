// NASA FIRMS port; mirrors src/widgets/fires/tests (the oracle) on the same fixture.
#include "../fires.hpp"
#include "../hcpcore.h"
#include "../text_util.hpp"
#include "test_util.hpp"
#include <string>
#include <vector>

using namespace hcptest;

int main() {
    using namespace hcp;
    const auto body = fixture("J1_VIIRS_C2_USA_contiguous_and_Hawaii_24h.csv");
    check(!body.empty(), "fixture present");
    const auto feed = fires::parse(body);
    check(feed && feed->size() == 40, "40 detections parsed");
    if (feed && feed->size() == 40) {
        const auto& d = *feed;
        check(d[0].frp_mw == 120.09 && d[0].latitude == 32.11736 && d[0].longitude == -86.79617, "strongest first");
        check(d[0].acquired_ms == *text::civil_ms(2026, 10, 13, 18, 38, 0), "acquisition time UTC");
        check(d[0].confidence == fires::Detection::Low && d[1].confidence == fires::Detection::Nominal, "confidence levels");
    }
    check(!fires::parse("") && !fires::parse("<html>") && !fires::parse("a,b\n1,2"), "non-FIRMS body rejected");
    const std::string head = "latitude,longitude,acq_date,acq_time,confidence,frp\n";
    const auto empty = fires::parse(head);
    check(empty && empty->empty(), "header only is valid and empty");
    const auto skipped = fires::parse(head + "x,1,2026-10-13,0054,h,1\n95,1,2026-10-13,0054,h,1\n1,1,bad,0054,h,1\n"
                                             "1,1,2026-10-13,0054,h\n1,1,2026-10-13,54,h,-1\n");
    check(skipped && skipped->empty(), "malformed rows skipped");
    const auto padded = fires::parse(head + "1,2,2026-10-13,54,high,3\r\n");
    check(padded && padded->size() == 1 && (*padded)[0].acquired_ms == *text::civil_ms(2026, 10, 13, 0, 54, 0) &&
              (*padded)[0].confidence == fires::Detection::High, "short time padded");

    std::vector<HcpFire> out(64);
    const int n = hcp_fires_parse(body.data(), body.size(), out.data(), out.size());
    check(n == 40 && out[0].frp_mw == 120.09 && out[0].lat == 32.11736 && out[0].lon == -86.79617 &&
              out[0].time_ms == *text::civil_ms(2026, 10, 13, 18, 38, 0) && out[0].high_confidence == 0, "C ABI rows");
    bool sorted = true;
    for (int i = 1; i < n; ++i) sorted = sorted && out[size_t(i - 1)].frp_mw >= out[size_t(i)].frp_mw;
    check(sorted, "C ABI strongest first");
    const std::string hi = head + "1,2,2026-10-13,54,high,3\n";
    check(hcp_fires_parse(hi.data(), hi.size(), out.data(), out.size()) == 1 && out[0].high_confidence == 1, "C ABI high confidence");
    check(hcp_fires_parse(body.data(), body.size(), out.data(), 5) == 5, "C ABI cap");
    check(hcp_fires_parse("a,b\n1,2", 7, out.data(), out.size()) == -1 && hcp_fires_parse(head.data(), head.size(), out.data(), 4) == 0,
          "C ABI rejects non-FIRMS, header-only is 0");
    return finish("fires");
}
