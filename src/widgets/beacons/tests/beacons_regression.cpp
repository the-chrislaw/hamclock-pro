#include <cmath>
// NCDXF/IARU beacon schedule: published rule (3 minute cycle from UTC minutes
// divisible by 3, 10 s slots, 4U1UN on 14.100 MHz in the first slot, each
// beacon moving up one band per slot). Offline.
#include "../beacons_model.hpp"
#include <QCoreApplication>
#include <QVariantMap>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    const std::int64_t base = 1760400000 - 1760400000 % 180; // a cycle start
    auto call = [](std::int64_t t, int band) { return std::string(ncdxf_beacon(ncdxf_current({t}, band).beacon_index).call); };

    check(call(base, 0) == "4U1UN", "cycle start: 4U1UN on 14.100");
    check(call(base + 9, 0) == "4U1UN", "still first slot at +9 s");
    check(call(base + 10, 0) == "VE8AT", "second slot: VE8AT on 14.100");
    check(call(base + 10, 1) == "4U1UN", "4U1UN moves up to 18.110 in slot 2");
    check(call(base + 40, 4) == "4U1UN", "4U1UN reaches 28.200 in slot 5");
    check(call(base + 170, 0) == "YV5B", "last slot on 14.100 is YV5B");
    check(call(base + 180, 0) == "4U1UN", "cycle repeats after 180 s");
    check(call(base + 20, 2) == "4U1UN", "4U1UN on 21.150 in slot 3");
    check(ncdxf_current({base + 3}, 0).seconds_remaining == 7, "7 s left at +3 s");
    check(ncdxf_current({base + 9}, 3).seconds_remaining == 1, "1 s left at +9 s");
    check(ncdxf_current({base}, 0).seconds_remaining == 10, "10 s left at slot start");
    // Five bands always show five distinct beacons.
    bool distinct = true;
    for (std::int64_t t = base; t < base + 180; t += 10)
        for (int a = 0; a < 5; ++a) for (int b = a + 1; b < 5; ++b)
            if (call(t, a) == call(t, b)) distinct = false;
    check(distinct, "five distinct beacons per slot");
    check(ncdxf_current({-1}, 0).beacon_index == 17, "negative time wraps");
    bool threw = false;
    try { ncdxf_current({base}, 5); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "band out of range throws");
    check(std::strcmp(ncdxf_beacon(0).call, "4U1UN") == 0 && ncdxf_band_khz(4) == 28200, "table");

    const QVariantList rows = beacon_rows({base + 10});
    check(rows.size() == 5, "five rows");
    check(rows[0].toMap()["band"].toString() == "14.100" && rows[0].toMap()["call"].toString() == "VE8AT", "row 0");
    check(rows[1].toMap()["call"].toString() == "4U1UN", "row 1");

    std::cerr << checks << " checks, " << failures << " failures\n";
    {
        const Location ny = ncdxf_beacon_location(0); // FN30as
        check(std::abs(ny.latitude - 40.73) < 0.05 && std::abs(ny.longitude + 74.0) < 0.1, "4U1UN locator centre near New York");
        const Location caracas = ncdxf_beacon_location(17); // FK60, 4 characters
        check(std::abs(caracas.latitude - 10.5) < 1e-9 && std::abs(caracas.longitude + 67.0) < 1e-9, "4-character locator uses the square centre");
        for (int i = 0; i < kNcdxfBeaconCount; ++i) {
            const Location l = ncdxf_beacon_location(i);
            check(l.latitude > -90 && l.latitude < 90 && l.longitude > -180 && l.longitude < 180, "beacon location in range");
        }
        const OverlayLayer layer = beacon_overlay_layer({base});
        check(layer.id == "beacons" && layer.markers.size() == 5, "overlay has the five on-air beacons");
        check(layer.markers[0].call == "4U1UN", "first marker is the 14.100 beacon");
        check(layer.markers[0].detail.startsWith("14.100 MHz"), "tap detail names the frequency");
    }
    return failures ? 1 : 0;
}
