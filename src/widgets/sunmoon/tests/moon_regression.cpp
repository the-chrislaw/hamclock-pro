// Moon ephemeris against published values (Meeus, Astronomical Algorithms
// 2nd ed., example 47.a; almanac new/full moon instants). Offline.
#include "../sun_moon_model.hpp"
#include "../../../domain/astro_moon.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <QTimeZone>
#include <cmath>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
std::int64_t utc(int y, int mo, int d, int h = 0, int mi = 0) {
    return QDateTime(QDate(y, mo, d), QTime(h, mi), QTimeZone::utc()).toSecsSinceEpoch();
}
double angdiff(double a, double b) { double d = std::fmod(a - b + 540.0, 360.0) - 180.0; return d; }
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;

    // Meeus 47.a: 1992-04-12 00:00 TD (JDE 2448724.5): lambda 133.162655,
    // beta -3.229126, distance 368409.7 km, RA 134.688470, Dec 13.768368.
    // TD = UTC + 59 s in 1992; the model adds 69 s, a 10 s (0.005 deg) offset.
    const auto p = moon_position({utc(1992, 4, 12, 0, 0) - 59});
    check(std::fabs(angdiff(p.longitude_deg, 133.162655)) < 0.1, "Meeus 47.a longitude");
    check(std::fabs(p.latitude_deg + 3.229126) < 0.05, "Meeus 47.a latitude");
    check(std::fabs(p.distance_km - 368409.7) < 300, "Meeus 47.a distance");
    check(std::fabs(angdiff(p.ra_deg, 134.688470)) < 0.15, "Meeus 47.a right ascension");
    check(std::fabs(p.dec_deg - 13.768368) < 0.1, "Meeus 47.a declination");

    // Almanac phases (elongation 0 at new moon, 180 at full moon).
    check(std::fabs(angdiff(moon_phase({utc(2000, 1, 6, 18, 14)}).elongation_deg, 0)) < 0.4, "new moon 2000-01-06 18:14");
    check(std::fabs(angdiff(moon_phase({utc(2000, 1, 21, 4, 40)}).elongation_deg, 180)) < 0.4, "full moon 2000-01-21 04:40");
    check(std::fabs(angdiff(moon_phase({utc(2025, 3, 14, 6, 55)}).elongation_deg, 180)) < 0.4, "full moon 2025-03-14 06:55");
    check(std::fabs(angdiff(moon_phase({utc(2025, 3, 29, 10, 58)}).elongation_deg, 0)) < 0.4, "new moon 2025-03-29 10:58");
    const auto full = moon_phase({utc(2025, 3, 14, 6, 55)});
    check(full.illuminated > 0.99 && std::string(full.name) == "Full Moon", "full moon illuminated and named");
    const auto fresh = moon_phase({utc(2025, 3, 29, 10, 58)});
    check(fresh.illuminated < 0.01 && std::string(fresh.name) == "New Moon", "new moon dark and named");
    const auto fq = moon_phase({utc(2025, 3, 6, 16, 32)}); // almanac first quarter
    check(fq.waxing && std::fabs(fq.illuminated - 0.5) < 0.05 && std::string(fq.name) == "First Quarter", "first quarter 2025-03-06");
    const auto lq = moon_phase({utc(2025, 3, 22, 11, 29)}); // almanac last quarter
    check(!lq.waxing && std::fabs(lq.illuminated - 0.5) < 0.05 && std::string(lq.name) == "Last Quarter", "last quarter 2025-03-22");

    // Rise/set: altitude at each event equals the horizon threshold; moon moves ~13 deg/day so
    // a mid-latitude day has about one rise and one set; sorted and inside the day.
    for (double lat : {-33.87, 0.0, 35.68, 51.5}) {
        for (double lon : {-122.4, 0.0, 151.2}) {
            const auto day = moon_events_for_utc_day(utc(2025, 3, 20), lat, lon);
            check(day.size() >= 1 && day.size() <= 3, "one to three moon events a day");
            for (std::size_t i = 0; i < day.size(); ++i) {
                const auto h = moon_horizontal(day[i].time, lat, lon);
                check(std::fabs(h.elevation_deg + 0.83) < 0.2, "topocentric centre at -0.83 deg (refraction plus semi-diameter) at event");
                check(day[i].time.value >= utc(2025, 3, 20) && day[i].time.value < utc(2025, 3, 21), "event inside day");
                if (i > 0) check(day[i].kind != day[i - 1].kind && day[i].time.value > day[i - 1].time.value, "events alternate and sort");
            }
        }
    }
    // Full moon rises near sunset: London 2025-03-14, moonrise ~ 17:50 UTC (sunset ~ 18:08 UTC), moonset after dawn.
    const auto lon_day = moon_events_for_utc_day(utc(2025, 3, 14), 51.5074, -0.1278);
    bool rise_evening = false;
    for (const auto& e : lon_day) if (e.kind == MoonEventKind::Rise && e.time.value > utc(2025, 3, 14, 16, 30) && e.time.value < utc(2025, 3, 14, 19, 0)) rise_evening = true;
    check(rise_evening, "full moon rises in the evening in London");
    const auto nxt = next_moon_event({utc(2025, 3, 14, 12, 0)}, MoonEventKind::Rise, 51.5074, -0.1278);
    check(nxt && nxt->value > utc(2025, 3, 14, 12, 0) && nxt->value < utc(2025, 3, 14, 19, 0), "next moonrise today");

    // Horizontal sanity: elevation in range, azimuth wraps, sub-lunar point is overhead.
    const auto t = UtcSeconds{utc(2025, 3, 14, 6, 55)};
    const auto pos = moon_position(t);
    const double gmst = std::fmod(280.46061837 + 360.98564736629 * ((t.value / 86400.0 + 2440587.5) - 2451545.0), 360.0);
    double sublon = std::fmod(pos.ra_deg - gmst + 540.0, 360.0) - 180.0;
    const auto overhead = moon_horizontal(t, pos.dec_deg, sublon);
    check(overhead.elevation_deg > 88.0, "moon near zenith at its sub-lunar point");
    const auto h = moon_horizontal(t, -33.87, 151.2);
    check(h.azimuth_deg >= 0 && h.azimuth_deg < 360 && h.elevation_deg >= -90 && h.elevation_deg <= 90, "az/el ranges");

    bool threw = false;
    try { moon_events_for_utc_day(0, 0, 181); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "longitude 181 rejected");
    threw = false;
    try { moon_horizontal({0}, std::nan(""), 0); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "NaN latitude rejected");

    // Summary text.
    const auto s = summarize_moon({utc(2025, 3, 14, 12, 0)}, 51.5074, -0.1278);
    check(s.phase == "Full Moon" && s.illumination.endsWith("%"), "summary phase and illumination");
    check(s.next_rise.endsWith("UTC") && s.next_set.endsWith("UTC"), "summary rise/set text");
    check(s.position.contains("Az") && s.position.contains("El"), "summary position text");

    // Disc inputs: elongation near 180 at full moon; hemisphere flag mirrors the lit side.
    check(std::fabs(angdiff(s.elongation_deg, 180)) < 5 && !s.southern, "summary elongation and northern flag");
    check(summarize_moon({utc(2025, 3, 14, 12, 0)}, -33.87, 151.21).southern, "southern observer flagged");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
