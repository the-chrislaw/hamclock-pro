// MUF map style: physical sanity of the 3000 km MUF image (offline).
#include "../muf_map.hpp"
#include "../path_model.hpp"
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
constexpr std::int64_t kNoonUtc = 1760356800; // 2026-10-13 12:00 UTC
} // namespace

int main() {
    using namespace hamclock;
    using namespace hamclock::bands;
    const UtcSeconds noon{kNoonUtc};
    const double day = muf_3000_mhz(150, 0, 0, noon), night = muf_3000_mhz(150, 0, 180, noon);
    check(day > 15 && day < 50, "daytime MUF plausible at SFI 150");
    check(night < day * 0.7, "night MUF lower than day");
    check(muf_3000_mhz(200, 0, 0, noon) > day, "MUF rises with SFI");
    check(muf_3000_mhz(150, 80, 0, noon) < day, "polar MUF lower");
    check(muf_3000_mhz(150, 400, 900, noon) > 0, "out-of-range coordinates stay finite");

    const QImage img = muf_image(150, noon);
    check(img.width() == 360 && img.height() == 180, "image is 360x180");
    check(img.pixelColor(180, 90).alpha() == 255, "pixels opaque");
    check(img.pixelColor(180, 90) != img.pixelColor(0, 90), "day and night differ");
    check(!muf_scale().stops.empty() && muf_scale().unit == "MHz", "legend present");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
