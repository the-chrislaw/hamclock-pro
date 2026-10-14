// Qt-free SWPC parsers; mirrors src/widgets/spaceweather/tests (the oracle) on the
// same real NOAA fixtures captured 2026-10-13.
#include "../json_mini.hpp"
#include "../swpc.hpp"
#include <cmath>
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
    // 2026-10-13T12:00:00Z
    const int64_t noon = 1791892800000LL;
    check(swpc::parse_iso_utc_ms("2026-10-13T12:00:00") == noon, "iso without Z");
    check(swpc::parse_iso_utc_ms("2026-10-13T12:00:00Z") == noon, "iso with Z");
    check(swpc::parse_iso_utc_ms("2026-10-13T12:00:00.250Z") == noon + 250, "fraction");
    check(!swpc::parse_iso_utc_ms("") && !swpc::parse_iso_utc_ms("2026-13-01T00:00:00") &&
          !swpc::parse_iso_utc_ms("garbage") && !swpc::parse_iso_utc_ms("2026-10-13T12:00:00+01:00"),
          "bad times rejected");
    check(swpc::parse_iso_utc_ms("1970-01-01T00:00:00Z") == 0, "epoch");

    const auto kp = swpc::latest_kp(fixture("noaa-planetary-k-index.json"));
    check(kp && kp->value == 3.0, "latest Kp");
    check(kp && kp->time_ms == noon, "Kp time is UTC");
    const auto sfi = swpc::latest_solar_flux(fixture("10cm-flux.json"));
    check(sfi && sfi->value == 123, "solar flux");
    const auto xray = swpc::latest_xray_flux(fixture("xrays-6-hour.json"));
    check(xray && std::abs(xray->value - 7.258945515786763e-07) < 1e-15, "X-ray 0.1-0.8nm only");
    check(xray && swpc::xray_class(xray->value) == "B7.2", "classifies B7.2");

    const auto day = swpc::xray_series(fixture("xrays-1-day.json"), 10);
    check(day.size() >= 140 && day.size() <= 145, "24 h of 10-minute points");
    check(day.size() > 1 && day.front().time_ms < day.back().time_ms, "oldest first");
    check(swpc::xray_series("[{\"time_tag\":\"2026-10-13T00:01:00Z\",\"flux\":1e-6,\"energy\":\"0.1-0.8nm\"},"
                            "{\"time_tag\":\"2026-10-13T00:05:00Z\",\"flux\":5e-6,\"energy\":\"0.1-0.8nm\"}]", 10)
                  .at(0).value == 5e-6, "bucket keeps the maximum");
    check(swpc::xray_series("garbage", 10).empty() && swpc::xray_series("[]", 0).empty(), "bad input -> empty");
    check(swpc::xray_plot_fraction(1e-8) == 0 && swpc::xray_plot_fraction(0) == 0 &&
          std::abs(swpc::xray_plot_fraction(1e-5) - 0.6) < 1e-9 && swpc::xray_plot_fraction(1) == 1, "plot fraction");

    for (const char* bad : {"", "garbage", "{}", "[{\"time_tag\":\"2026-10-13T00:00:00\",\"Kp\":11}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"Kp\":\"3\"}]", "[{\"Kp\":3}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"Kp\":3},]", "[1,2"})
        check(!swpc::latest_kp(bad), "malformed Kp rejected");
    check(!swpc::latest_xray_flux("[{\"time_tag\":\"2026-10-13T00:00:00Z\",\"flux\":1e-6,\"energy\":\"0.05-0.4nm\"}]"),
          "other energy band ignored");
    check(swpc::latest_kp("[{\"time_tag\":\"2026-10-13T03:00:00\",\"Kp\":2},{\"time_tag\":\"2026-10-13T00:00:00\",\"Kp\":7}]")->value == 2,
          "latest by time, not order");

    check(swpc::xray_class(1e-8) == "A1.0" && swpc::xray_class(9.99e-8) == "A9.9", "A class");
    check(swpc::xray_class(1e-7) == "B1.0" && swpc::xray_class(3.2e-6) == "C3.2", "B, C class");
    check(swpc::xray_class(5.5e-5) == "M5.5" && swpc::xray_class(1e-4) == "X1.0", "M, X class");
    check(swpc::xray_class(1.2e-3) == "X12", "X10+ integer");
    check(swpc::xray_class(0).empty() && swpc::xray_class(NAN).empty(), "invalid flux has no class");
    check(swpc::kp_description(3) == "Quiet" && swpc::kp_description(4.33) == "Active" &&
          swpc::kp_description(5) == "Minor storm (G1)" && swpc::kp_description(9) == "Extreme storm (G5)", "Kp text");

    const auto j = json::parse("{\"a\":[1,2.5e1,\"\\u00e9\\ud83d\\ude00\",true,null],\"b\":{}}");
    check(j && j->at("a").array.size() == 5 && j->at("a").array[1].number == 25 &&
          j->at("a").array[2].string == "\xC3\xA9\xF0\x9F\x98\x80" && j->at("zz").type == json::Value::Type::Null,
          "json basics and unicode");
    check(!json::parse("[1] x") && !json::parse("[01]") && !json::parse("\"\\ud800\""), "json strictness");
    check(!json::parse(std::string(200, '[')), "depth limit");

    if (failures) { std::cerr << failures << " failure(s)\n"; return 1; }
    std::cout << "ok\n";
    return 0;
}
