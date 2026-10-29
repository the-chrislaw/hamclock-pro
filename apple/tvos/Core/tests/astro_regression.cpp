// Planets and solar eclipses through the C ABI; values from src/widgets/planets/tests and
// src/widgets/eclipse/tests (the oracle, published almanac/NASA figures).
#include "../hcpcore.h"
#include "../text_util.hpp"
#include "test_util.hpp"
#include <cstdlib>
#include <string>

using namespace hcptest;

namespace {
constexpr int kMars = 2, kJupiter = 3, kVenus = 1;
int64_t utc(int y, int mo, int d, int h = 0, int mi = 0, int s = 0) { return *hcp::text::civil_ms(y, mo, d, h, mi, s) / 1000; }

double elevation(int planet, int64_t t, double lat, double lon) {
    HcpPlanet p{};
    return hcp_planet(planet, t, lat, lon, &p) ? p.el_deg : 999;
}

// Brute force: first 10 s sample after t where the planet is up (rise) or down (set).
int64_t scan_event(int planet, int64_t t, bool rise, double lat, double lon) {
    bool was = rise ? elevation(planet, t, lat, lon) >= 0 : elevation(planet, t, lat, lon) < 0;
    for (int64_t s = t + 10; s <= t + 48 * 3600; s += 10) {
        const bool now = rise ? elevation(planet, s, lat, lon) >= 0 : elevation(planet, s, lat, lon) < 0;
        if (now && !was) return s;
        was = now;
    }
    return 0;
}

void planets() {
    HcpPlanet p{};
    check(hcp_planet(kMars, utc(2025, 1, 16), 0, 0, &p) == 1 && std::string(p.name) == "Mars", "Mars by index");
    check(p.ra_deg > 100 && p.ra_deg < 130 && p.dec_deg > 20 && p.dec_deg < 28, "Mars in Gemini at opposition");
    check(hcp_planet(kMars, utc(2025, 1, 12), 0, 0, &p) && near(p.distance_au, 0.642, 0.01), "Mars distance Jan 2025");
    check(hcp_planet(kVenus, utc(2025, 6, 1), 0, 0, &p) && near(p.elongation_deg, 45.9, 0.5), "Venus 45.9 deg on 2025-06-01");
    check(hcp_planet(kJupiter, utc(2026, 1, 10), 0, 0, &p) && p.distance_au > 4.1 && p.distance_au < 4.3, "Jupiter opposition distance");
    check(hcp_planet(6, utc(2025, 9, 23), 0, 0, &p) && std::string(p.name) == "Neptune" && near(p.distance_au, 28.9, 0.3),
          "Neptune distance at opposition");
    HcpPlanet sub{};
    check(hcp_planet(kMars, utc(2025, 1, 16, 5), 0, 0, &sub) && hcp_planet(kMars, utc(2025, 1, 16, 5), sub.sub_lat, sub.sub_lon, &p) &&
              p.el_deg > 89.0 && near(sub.sub_lat, p.dec_deg, 0.5), "planet overhead at its subpoint");
    check(!hcp_planet(7, 0, 0, 0, &p) && !hcp_planet(-1, 0, 0, 0, &p) && !hcp_planet(0, 0, 91, 0, &p) &&
              !hcp_planet(0, 0, 0, 0, nullptr), "bad index / station rejected");

    // Mars at opposition (2025-01-16) rises around sunset and sets around sunrise: London.
    const double lat = 51.5, lon = -0.13;
    const int64_t t0 = utc(2025, 1, 16, 12);
    int64_t rise = 0, set = 0, sunset = 0, sunrise = 0;
    check(hcp_next_planet_event(kMars, t0, 0, lat, lon, &rise) && hcp_next_planet_event(kMars, t0, 1, lat, lon, &set),
          "Mars rise and set found");
    check(hcp_next_sun_event(t0, 1, lat, lon, &sunset) && hcp_next_sun_event(t0, 0, lat, lon, &sunrise), "sun events");
    check(std::llabs(rise - sunset) < 2 * 3600 && std::llabs(set - sunrise) < 2 * 3600, "opposition: rise ~ sunset, set ~ sunrise");
    check(rise > t0 && rise < set, "rise before set, both after start");
    check(std::fabs(elevation(kMars, rise, lat, lon)) < 0.05 && std::fabs(elevation(kMars, set, lat, lon)) < 0.05,
          "elevation ~0 at the events");
    check(elevation(kMars, rise - 600, lat, lon) < 0 && elevation(kMars, rise + 600, lat, lon) > 0, "rising through the horizon");
    check(std::llabs(rise - scan_event(kMars, t0, true, lat, lon)) <= 10 &&
              std::llabs(set - scan_event(kMars, t0, false, lat, lon)) <= 10, "matches a 10 s brute-force scan");
    int64_t again = 0;
    check(hcp_next_planet_event(kMars, rise, 0, lat, lon, &again) && again > rise + 20 * 3600, "strictly after: next day's rise");
    // Circumpolar: at 80 N in January Mars (dec ~+25) never sets.
    check(!hcp_next_planet_event(kMars, t0, 1, 80, 0, &set) && !hcp_next_planet_event(kMars, t0, 2, lat, lon, &set) &&
              !hcp_next_planet_event(9, t0, 0, lat, lon, &set) && !hcp_next_planet_event(kMars, t0, 0, lat, lon, nullptr),
          "no event in window / bad input -> 0");
}

void eclipses() {
    HcpEclipse e{};
    // 2024-04-08 total, greatest 18:17:16 UTC, gamma 0.3431 (NASA), seen from Dallas.
    check(hcp_next_solar_eclipse(utc(2024, 1, 1), 32.78, -96.80, &e) == 1 && e.valid, "2024 eclipse found");
    check(std::llabs(e.greatest_s - utc(2024, 4, 8, 18, 17, 16)) <= 300 && near(e.gamma, 0.3431, 0.01) &&
              std::string(e.type) == "Total", "greatest, gamma, type");
    check(e.visibility == 2 && e.local_magnitude > 0.95, "Dallas sees it near total");
    check(std::llabs(e.local_max_s - utc(2024, 4, 8, 18, 42)) <= 300, "Dallas max time");
    check(std::llabs(e.local_start_s - utc(2024, 4, 8, 17, 23)) <= 480 && std::llabs(e.local_end_s - utc(2024, 4, 8, 20, 2)) <= 480,
          "Dallas contacts");
    check(hcp_next_solar_eclipse(utc(2024, 1, 1), 39.74, -104.99, &e) && e.visibility == 2 && e.local_magnitude > 0.6 &&
              e.local_magnitude < 0.9 && e.local_start_s < e.local_max_s && e.local_max_s < e.local_end_s, "Denver partial");
    check(hcp_next_solar_eclipse(utc(2024, 1, 1), 51.5, -0.13, &e) && e.visibility == 0 && e.local_magnitude == 0 &&
              e.local_start_s == 0, "London: not visible");
    check(hcp_next_solar_eclipse(utc(2024, 1, 1), NAN, NAN, &e) && e.valid && e.visibility == 0 && e.local_max_s == 0,
          "no station -> visibility 0");
    check(hcp_next_solar_eclipse(utc(2025, 1, 1), NAN, NAN, &e) && std::string(e.type) == "Partial" &&
              std::llabs(e.greatest_s - utc(2025, 3, 29, 10, 48, 36)) <= 300 && e.magnitude > 0, "2025-03-29 partial");
    check(hcp_next_solar_eclipse(utc(2026, 3, 1), NAN, NAN, &e) && std::string(e.type) == "Total" &&
              std::llabs(e.greatest_s - utc(2026, 8, 12, 17, 46, 6)) <= 300, "2026-08-12 total");
    check(!hcp_next_solar_eclipse(0, 91, 0, &e) && !e.valid && !hcp_next_solar_eclipse(0, 0, 0, nullptr), "bad station rejected");
}
} // namespace

int main() {
    planets();
    eclipses();
    return finish("astro");
}
