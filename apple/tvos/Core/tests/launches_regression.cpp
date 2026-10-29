// Launch Library 2 port; mirrors src/widgets/launches/tests (the oracle) on the same fixture.
#include "../hcpcore.h"
#include "../launches.hpp"
#include "../text_util.hpp"
#include "test_util.hpp"
#include <string>
#include <vector>

using namespace hcptest;

namespace {
int64_t utc(int y, int mo, int d, int h, int mi) { return *hcp::text::civil_ms(y, mo, d, h, mi, 0); }
} // namespace

int main() {
    using namespace hcp;
    const auto body = fixture("upcoming");
    check(!body.empty(), "fixture present");
    const auto feed = launches::parse(body);
    check(feed && feed->size() == 6, "6 launches parsed");
    if (feed && feed->size() == 6) {
        const auto& l = *feed;
        check(l.front().name.rfind("Zhuque-2E", 0) == 0 && l.front().provider == "LandSpace", "first row");
        check(l[1].net_ms == utc(2026, 10, 15, 23, 44) && l[1].status == "Go" && l[1].precise, "second row time and status");
        check(!l.back().precise && l.back().status == "TBD", "date-only launch is approximate");
        check(launches::ham_hint(l[2]).rfind("Rideshare", 0) == 0, "Bandwagon rideshare hinted");
        check(launches::ham_hint(l[3]).empty() && launches::ham_hint(l[4]).empty(), "Starlink and MMX not hinted");
        const auto vsfb = launches::pad_position(l[1]);
        check(vsfb && near(vsfb->lat, 34.73, 0.1) && vsfb->lon < -120, "Vandenberg pad position");
        check(launches::pad_position(l[0]).has_value(), "Jiuquan known");
    }
    launches::Launch iss;
    iss.name = "Falcon 9 | Crew-14";
    check(launches::ham_hint(iss) == "ISS visit (ARISS)", "crew flight hinted");
    launches::Launch cube;
    cube.name = "Electron | Amateur radio cubesat";
    check(launches::ham_hint(cube) == "Amateur/cubesat payloads", "amateur payload hinted");
    launches::Launch nowhere;
    nowhere.location = "Somewhere Unlisted";
    check(!launches::pad_position(nowhere), "unknown site gives no position");
    check(!launches::parse("") && !launches::parse("{}"), "non-list rejected");
    const auto bad = launches::parse(R"({"results":[{"name":"","net":"2026-01-01T00:00:00Z"},{"name":"x","net":"junk"}]})");
    check(bad && bad->empty(), "malformed rows skipped");

    const auto now = utc(2026, 10, 14, 20, 0);
    check(launches::countdown_text(utc(2026, 10, 15, 23, 44), now) == "T-27h 44m", "countdown hours");
    check(launches::countdown_text(utc(2026, 10, 19, 10, 30), now) == "T-4d 14h", "countdown days");
    check(launches::countdown_text(utc(2026, 10, 14, 20, 12), now) == "T-12m", "countdown minutes");
    check(launches::countdown_text(utc(2026, 10, 14, 19, 0), now) == "T+0m", "past launch");

    // C ABI.
    std::vector<HcpLaunch> out(16);
    const int n = hcp_launches_parse(body.data(), body.size(), out.data(), out.size());
    check(n == 6, "C ABI count");
    if (n == 6) {
        check(std::string(out[1].provider) == "SpaceX" && std::string(out[1].status) == "Go" && out[1].precise &&
                  out[1].net_ms == utc(2026, 10, 15, 23, 44), "C ABI row fields");
        check(std::string(out[1].location) == "Vandenberg SFB, CA, USA" && out[1].has_pad && near(out[1].pad_lat, 34.73, 1e-9),
              "C ABI pad position");
        check(std::string(out[2].hint).rfind("Rideshare", 0) == 0 && out[3].hint[0] == '\0', "C ABI hints");
        check(std::string(out[1].mission) == "USSF-481" && std::string(out[1].pad) == "Space Launch Complex 4E", "C ABI mission/pad");
    }
    check(hcp_launches_parse(body.data(), body.size(), out.data(), 2) == 2, "C ABI cap");
    check(hcp_launches_parse("{}", 2, out.data(), out.size()) == -1, "C ABI rejects non-list");
    char buf[16];
    check(hcp_launch_countdown(utc(2026, 10, 15, 23, 44), now, buf, sizeof buf) && std::string(buf) == "T-27h 44m",
          "C ABI countdown");
    return finish("launches");
}
