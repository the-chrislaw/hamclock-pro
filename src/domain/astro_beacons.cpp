#include <cctype>
#include <string>
#include "astro_beacons.hpp"
#include <stdexcept>
namespace hamclock {
namespace {
constexpr std::array<NcdxfBeacon, kNcdxfBeaconCount> kBeacons{{
    {"4U1UN", "United Nations, New York", "FN30as"},
    {"VE8AT", "Eureka, Canada", "EQ79ax"},
    {"W6WX", "Mt Umunhum, California", "CM97bg"},
    {"KH6RS", "Maui, Hawaii", "BL10ts"},
    {"ZL6B", "Masterton, New Zealand", "RE78tw"},
    {"VK6RBP", "Rolystone, Australia", "OF87av"},
    {"JA2IGY", "Mt Asama, Japan", "PM84jk"},
    {"RR9O", "Novosibirsk, Russia", "NO14kx"},
    {"VR2B", "Hong Kong", "OL72bm"},
    {"4S7B", "Colombo, Sri Lanka", "MJ96wv"},
    {"ZS6DN", "Pretoria, South Africa", "KG33xu"},
    {"5Z4B", "Kenya", "KI88hs"},
    {"4X6TU", "Tel Aviv, Israel", "KM72jb"},
    {"OH2B", "Lohja, Finland", "KP20eh"},
    {"CS3B", "Madeira", "IM12or"},
    {"LU4AA", "Buenos Aires, Argentina", "GF05tj"},
    {"OA4B", "Lima, Peru", "FH17mw"},
    {"YV5B", "Caracas, Venezuela", "FK60"},
}};
constexpr double kBandKhz[kNcdxfBandCount] = {14100, 18110, 21150, 24930, 28200};
}
const NcdxfBeacon& ncdxf_beacon(int index) {
    if (index < 0 || index >= kNcdxfBeaconCount) throw std::out_of_range("beacon index");
    return kBeacons[static_cast<std::size_t>(index)];
}
double ncdxf_band_khz(int band) {
    if (band < 0 || band >= kNcdxfBandCount) throw std::invalid_argument("beacon band");
    return kBandKhz[band];
}
BeaconSlot ncdxf_current(UtcSeconds utc, int band) {
    if (band < 0 || band >= kNcdxfBandCount) throw std::invalid_argument("beacon band");
    std::int64_t in_cycle = utc.value % 180;
    if (in_cycle < 0) in_cycle += 180;
    const int slot = static_cast<int>(in_cycle / 10);
    return {((slot - band) % kNcdxfBeaconCount + kNcdxfBeaconCount) % kNcdxfBeaconCount,
            static_cast<int>(10 - in_cycle % 10)};
}
Location ncdxf_beacon_location(int index) {
    const std::string loc = ncdxf_beacon(index).locator;
    if (loc.size() != 4 && loc.size() != 6) throw std::invalid_argument("beacon locator length");
    double lon = (std::toupper(loc[0]) - 'A') * 20.0 - 180.0 + (loc[2] - '0') * 2.0;
    double lat = (std::toupper(loc[1]) - 'A') * 10.0 - 90.0 + (loc[3] - '0') * 1.0;
    if (loc.size() == 6) {
        lon += (std::toupper(loc[4]) - 'A') * (2.0 / 24.0) + 1.0 / 24.0;
        lat += (std::toupper(loc[5]) - 'A') * (1.0 / 24.0) + 1.0 / 48.0;
    } else {
        lon += 1.0;
        lat += 0.5;
    }
    return {lat, lon};
}
}
