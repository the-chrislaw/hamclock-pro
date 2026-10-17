// Qt-free NHC storms parser; mirrors src/widgets/storms/tests (the oracle) on the same fixtures.
#include "../hcpcore.h"
#include "../storms.hpp"
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
    const auto body = fixture("CurrentStorms.json");
    check(!body.empty(), "fixture present");
    const auto feed = storms::parse(body);
    check(feed && feed->size() == 2, "2 storms parsed");
    if (feed && feed->size() == 2) {
        const auto& s = (*feed)[0];
        check(s.name == "Simon" && s.classification == "HU" && s.wind_kt == 80, "strongest first");
        check(s.pressure_mb == 977 && s.latitude == 20.2 && s.longitude == -107.5, "position and pressure");
        check(s.heading_deg == 260 && s.speed_kt == 2, "movement");
        check(s.issued_ms == 1791925200000LL, "issuance 2026-10-13T21:00Z");
        check((*feed)[1].name == "Trudy" && (*feed)[1].id == "ep212026", "second storm");
    }
    check(storms::category(80) == 1 && storms::category(63) == 0 && storms::category(137) == 5 &&
          storms::category(100) == 3, "categories");
    check(storms::compass(260) == "W" && storms::compass(320) == "NW" && storms::compass(359) == "N" &&
          storms::compass(-1).empty(), "compass");
    check(storms::classification_text("TS") == "Tropical storm" && storms::classification_text("??") == "Tropical cyclone",
          "class text");

    const auto idx4 = storms::parse(fixture("CurrentStorms-adv4.json"));
    check(idx4 && idx4->size() == 1, "adv4 index");
    check(!storms::parse("") && !storms::parse("[]") && !storms::parse("{}"), "non-index rejected");
    const auto empty = storms::parse(R"({"activeStorms":[]})");
    check(empty && empty->empty(), "empty list is valid");
    const auto numeric = storms::parse(R"({"activeStorms":[{"name":" X ","intensity":45,"latitudeNumeric":"10.5","longitudeNumeric":-99,"movementDir":400},
        {"name":"","intensity":"30","latitudeNumeric":1,"longitudeNumeric":1},
        {"name":"Bad","intensity":"30","latitudeNumeric":95,"longitudeNumeric":1}]})");
    check(numeric && numeric->size() == 1 && (*numeric)[0].name == "X" && (*numeric)[0].latitude == 10.5 &&
          (*numeric)[0].heading_deg == -1 && (*numeric)[0].issued_ms == 0, "numeric forms; bad rows skipped");

    HcpStorm out[4];
    check(hcp_storms_parse(body.data(), body.size(), out, 4) == 2 && out[0].wind_kt == 80 && out[0].category == 1,
          "C ABI");
    check(hcp_storms_parse("{}", 2, out, 4) == -1, "C ABI rejects non-index");
    return failures ? 1 : 0;
}
