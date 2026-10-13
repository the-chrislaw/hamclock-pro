#pragma once
#include "clock_state.hpp"
#include <array>
#include <cstdint>
namespace hamclock {
// NCDXF/IARU International Beacon Project: 18 beacons share each of five
// frequencies in a repeating 3 minute cycle of ten second slots, starting at
// UTC minutes divisible by 3. At slot s the beacon on band b (0 = 14.100) is
// (s - b) mod 18, so each beacon steps up one band every 10 s. Offline.
struct NcdxfBeacon {
    const char* call;
    const char* location;
    const char* locator; // approximate Maidenhead (published beacon list)
};
constexpr int kNcdxfBeaconCount = 18;
constexpr int kNcdxfBandCount = 5;
const NcdxfBeacon& ncdxf_beacon(int index);
double ncdxf_band_khz(int band);

struct BeaconSlot {
    int beacon_index;      // into the table
    int seconds_remaining; // 1..10 until the slot ends
};
// band outside 0..4 throws std::invalid_argument.
BeaconSlot ncdxf_current(UtcSeconds utc, int band);
}
