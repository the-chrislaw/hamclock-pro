// On-the-air spots port; mirrors src/widgets/ota/tests (the oracle) on the same fixtures.
#include "../hcpcore.h"
#include "../ota.hpp"
#include "../text_util.hpp"
#include "test_util.hpp"
#include <string>
#include <vector>

using namespace hcptest;

namespace {
int64_t utc(int y, int mo, int d, int h, int mi, int s) { return *hcp::text::civil_ms(y, mo, d, h, mi, s); }
} // namespace

int main() {
    using namespace hcp;
    const auto pota_body = fixture("activator.json");
    const auto sota_body = fixture("all.json");
    const auto wwff_body = fixture("wwff.json");
    check(!pota_body.empty() && !sota_body.empty() && !wwff_body.empty(), "fixtures present");

    const auto pota = ota::parse_pota(pota_body);
    check(pota && pota->size() == 6, "POTA: 6 live spots (QRT dropped)");
    if (pota && !pota->empty()) {
        const auto& first = pota->front();
        check(first.call == "K3AV" || first.call == "K5G", "POTA newest first");
        check(first.time_ms == utc(2026, 10, 13, 17, 41, 38), "POTA time is UTC");
        check(first.program == "POTA" && first.freq_khz > 0 && first.has_position, "POTA fields");
    }
    const auto sota = ota::parse_sota(sota_body);
    check(sota && sota->size() == 8, "SOTA: 8 spots");
    if (sota && !sota->empty()) {
        check(sota->front().call == "KQ4MHE" && sota->front().reference == "W4C/CM-009", "SOTA newest");
        check(sota->front().freq_khz == 14064.0, "SOTA MHz converted to kHz");
    }
    const auto wwff = ota::parse_wwff(wwff_body);
    check(wwff && wwff->size() == 7, "WWFF: 7 spots (QRT dropped)");
    if (wwff && !wwff->empty()) {
        const auto& w = wwff->front();
        check(w.call == "VK3PF" && w.reference == "VKFF-0753" && w.program == "WWFF" && w.freq_khz == 7165.0 &&
                  w.name == "Bunyip State Park" && w.has_position, "WWFF fields");
        check(w.time_ms == utc(2026, 10, 17, 8, 17, 1), "WWFF epoch is UTC");
    }
    const auto wbad = ota::parse_wwff("[{\"activator\":\"A1B\",\"reference\":\"KFF-1\"}]");
    check(!ota::parse_wwff("{}") && wbad && wbad->empty(), "WWFF malformed rows skipped");
    check(!ota::parse_pota("") && !ota::parse_pota("{}") && !ota::parse_sota("nope"), "non-arrays rejected");
    const auto skipped = ota::parse_pota(
        R"([{"activator":"","reference":"US-1","frequency":"7000","spotTime":"2026-10-13T17:00:00"},
            {"activator":"A1B","reference":"US-1","frequency":"x","spotTime":"2026-10-13T17:00:00"},
            {"activator":"A1B","reference":"US-1","frequency":"7000","spotTime":"junk"},
            {"activator":"A1B","reference":"US-1","frequency":"7000","spotTime":"2026-10-13T17:00:00","comments":"QRT"}])");
    check(skipped && skipped->empty(), "malformed rows skipped");
    const auto zoned = ota::parse_sota(R"([{"activatorCallsign":" ab1cd ","summitCode":"W1/AB-001","frequency":"7.032",)"
                                       R"("mode":"cw","timeStamp":"2026-10-13T19:00:00+02:00"}])");
    check(zoned && zoned->size() == 1 && (*zoned)[0].call == "AB1CD" && (*zoned)[0].mode == "CW" &&
              (*zoned)[0].time_ms == utc(2026, 10, 13, 17, 0, 0) && !(*zoned)[0].has_position, "zoned time, trims and upper-cases");

    check(ota::band_for_khz(14064) == "20m" && ota::band_for_khz(7165) == "40m" && ota::band_for_khz(144200) == "2m" &&
              ota::band_for_khz(9000).empty(), "band table");

    // C ABI.
    std::vector<HcpOtaSpot> out(16);
    const int np = hcp_pota_parse(pota_body.data(), pota_body.size(), out.data(), out.size());
    check(np == 6 && std::string(out[0].program) == "POTA" && out[0].time_ms == utc(2026, 10, 13, 17, 41, 38) &&
              out[0].has_position, "C ABI POTA");
    bool newest_first = true;
    for (int i = 1; i < np; ++i) newest_first = newest_first && out[size_t(i - 1)].time_ms >= out[size_t(i)].time_ms;
    check(newest_first, "C ABI newest first");
    const int ns = hcp_sota_parse(sota_body.data(), sota_body.size(), out.data(), out.size());
    check(ns == 8 && std::string(out[0].call) == "KQ4MHE" && std::string(out[0].reference) == "W4C/CM-009" &&
              out[0].freq_khz == 14064.0 && std::string(out[0].mode) == "CW" && std::string(out[0].name) == "Big Butt",
          "C ABI SOTA");
    const int nw = hcp_wwff_parse(wwff_body.data(), wwff_body.size(), out.data(), out.size());
    check(nw == 7 && std::string(out[0].name) == "Bunyip State Park" && near(out[0].lat, -37.9957, 1e-9) &&
              std::string(out[0].comment) == "also SOTA VK3/VC-037 CQ", "C ABI WWFF");
    check(hcp_wwff_parse(wwff_body.data(), wwff_body.size(), out.data(), 3) == 3, "C ABI cap");
    check(hcp_pota_parse("{}", 2, out.data(), out.size()) == -1 && hcp_sota_parse("nope", 4, out.data(), out.size()) == -1 &&
              hcp_wwff_parse(nullptr, 0, out.data(), out.size()) == -1, "C ABI rejects non-arrays");
    char buf[8];
    check(hcp_band_name(14064, buf, sizeof buf) && std::string(buf) == "20m", "C ABI band name");
    check(hcp_band_name(432100, buf, sizeof buf) && std::string(buf) == "70cm" && !hcp_band_name(9000, buf, sizeof buf) &&
              buf[0] == '\0', "C ABI band outside");
    return finish("ota");
}
