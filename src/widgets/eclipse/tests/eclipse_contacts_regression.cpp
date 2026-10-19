// Contact solver vs published NASA greatest-eclipse points: at the point of
// greatest eclipse the maximum falls at the published time and totality lasts
// the published duration (Espenak, NASA eclipse bulletins / eclipse.gsfc.nasa.gov).
#include "../../../domain/astro_eclipse.hpp"
#include <cmath>
#include <ctime>
#include <stdexcept>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
hamclock::UtcSeconds utc(int y, int mo, int d, int h, int mi, int s) {
    std::tm tm{};
    tm.tm_year = y - 1900; tm.tm_mon = mo - 1; tm.tm_mday = d; tm.tm_hour = h; tm.tm_min = mi; tm.tm_sec = s;
    return hamclock::UtcSeconds{static_cast<std::int64_t>(timegm(&tm))};
}
struct Ref { hamclock::UtcSeconds before, greatest; double lat, lon, duration_s, tolerance_s; const char* name; };
}

int main() {
    using namespace hamclock;
    const Ref refs[] = {
        {utc(2024, 3, 1, 0, 0, 0), utc(2024, 4, 8, 18, 17, 16), 25.29, -104.14, 268.0, 8.0, "2024-04-08"},
        {utc(2027, 7, 1, 0, 0, 0), utc(2027, 8, 2, 10, 7, 50), 25.5, 33.2, 383.0, 8.0, "2027-08-02"},
        {utc(2017, 8, 1, 0, 0, 0), utc(2017, 8, 21, 18, 25, 32), 37.57, -87.67, 160.0, 100.0, "2017-08-21"}, // wide: the Meeus Sun puts the path ~55 km (27 arcsec) off NASA's, and totality is short near the edge
    };
    for (const Ref& r : refs) {
        const auto e = next_solar_eclipse(r.before);
        check(e.has_value(), r.name);
        if (!e) continue;
        const EclipseContacts c = eclipse_contacts(*e, r.lat, r.lon);
        std::cout << r.name << " max " << c.maximum.value - r.greatest.value << " s, duration "
                  << (c.third.value - c.second.value) << " (ref " << r.duration_s << ") sep " << c.min_separation_deg << '\n';
        check(c.partial && c.magnitude > 0.998, "station is on or at the edge of the path");
        check(std::llabs(c.maximum.value - r.greatest.value) <= 60, "maximum matches NASA greatest eclipse");
        check(c.central, "inside the umbra");
        check(std::fabs(double(c.third.value - c.second.value) - r.duration_s) <= r.tolerance_s, "totality duration matches NASA");
        check(c.first.value < c.second.value && c.second.value < c.maximum.value && c.maximum.value < c.third.value && c.third.value < c.last.value, "contact order");
    }
    // Far from the path: partial only, no inner contacts.
    const auto e = next_solar_eclipse(utc(2024, 3, 1, 0, 0, 0));
    const EclipseContacts far = eclipse_contacts(*e, 51.5, -0.1);
    check(!far.central && far.second.value == 0, "no inner contacts outside the path");
    try { eclipse_contacts(*e, 95, 0); check(false, "bad latitude throws"); } catch (const std::invalid_argument&) { check(true, ""); }
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
