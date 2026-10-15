// Qt-free USGS quakes parser; mirrors src/widgets/quakes/tests (the oracle) on the same fixture.
#include "../hcpcore.h"
#include "../quakes.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
std::string fixture(const char* name) {
    std::ifstream in(std::string(HCP_FIXTURE_DIR) + "/" + name, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
} // namespace

int main() {
    using namespace hcp;
    const auto body = fixture("2.5_day.geojson");
    check(!body.empty(), "fixture present");
    const auto feed = quakes::parse(body);
    check(feed && feed->size() == 8, "8 quakes parsed");
    if (!feed || feed->empty()) return 1;
    bool newest_first = true;
    for (size_t i = 1; i < feed->size(); ++i) newest_first = newest_first && (*feed)[i - 1].time_ms >= (*feed)[i].time_ms;
    check(newest_first, "newest first");
    const auto* top = quakes::strongest(*feed);
    check(top && top->magnitude == 6.8 && top->place == "central Mid-Atlantic Ridge", "strongest quake");
    check(top && top->latitude == -0.4085 && top->longitude == -19.4308 && top->depth_km == 10, "lat/lon/depth order");
    check(feed->front().place == "126 km S of Kokopo, Papua New Guinea", "newest is Kokopo");
    check(quakes::magnitude_class(6.8) == "Strong" && quakes::magnitude_class(7.1) == "Major" &&
          quakes::magnitude_class(5.2) == "Moderate" && quakes::magnitude_class(2.5) == "Minor", "classes");

    check(!quakes::parse("") && !quakes::parse("[]") && !quakes::parse("{}"), "non-feed rejected");
    const auto skipped = quakes::parse(
        R"({"type":"FeatureCollection","features":[
        {"properties":{"mag":null,"time":1},"geometry":{"coordinates":[1,2,3]}},
        {"properties":{"mag":3,"time":"x"},"geometry":{"coordinates":[1,2,3]}},
        {"properties":{"mag":3,"time":1},"geometry":{"coordinates":[1]}},
        {"properties":{"mag":3,"time":1},"geometry":{"coordinates":[200,2,3]}}]})");
    check(skipped && skipped->empty() && !quakes::strongest(*skipped), "malformed rows skipped");

    HcpQuake q[3];
    check(hcp_quakes_parse(body.data(), body.size(), q, 3) == 3 && q[0].magnitude == 6 &&
          std::string(q[0].place) == "126 km S of Kokopo, Papua New Guinea", "C ABI list, capped");
    const int idx = hcp_quakes_strongest_index(body.data(), body.size());
    check(idx >= 0 && (*feed)[static_cast<size_t>(idx)].magnitude == 6.8, "C ABI strongest index");
    char buf[16];
    check(hcp_quake_magnitude_class(6.8, buf, sizeof buf) && std::string(buf) == "Strong", "C ABI class");
    check(hcp_quakes_parse("garbage", 7, q, 3) == -1 && hcp_quakes_parse(nullptr, 0, q, 3) == -1, "C ABI bad input");
    return failures ? 1 : 0;
}
