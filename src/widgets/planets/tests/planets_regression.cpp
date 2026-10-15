// Planet ephemeris checked against published almanac events: oppositions are
// where the geocentric ecliptic longitude differs from the Sun's by 180 degrees,
// conjunctions where it equals it. Dates from the usual almanac listings.
#include "../planets_model.hpp"
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
std::int64_t utc(int y, int mo, int d, int h = 0) {
    return QDateTime(QDate(y, mo, d), QTime(h, 0), QTimeZone::utc()).toSecsSinceEpoch();
}
double wrap180(double d) { d = std::fmod(d + 180.0, 360.0); if (d < 0) d += 360.0; return d - 180.0; }
// Time (days from `centre`) when the planet's ecliptic longitude minus the Sun's
// crosses `target` degrees (180 = opposition, 0 = conjunction), found by scanning.
double crossing_day(hamclock::Planet p, std::int64_t centre, double target) {
    double best = 1e9, bestT = 0;
    for (std::int64_t t = centre - 10 * 86400; t <= centre + 10 * 86400; t += 600) {
        const auto q = hamclock::planet_position(p, {t});
        const double d = std::fabs(wrap180(q.ecliptic_lon_deg - q.sun_lon_deg - target));
        if (d < best) { best = d; bestT = static_cast<double>(t); }
    }
    return (bestT - static_cast<double>(centre)) / 86400.0;
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    (void)wrap180;

    // Oppositions (ecliptic longitude 180 from the Sun, within 2 days of the published date).
    check(std::fabs(crossing_day(Planet::Mars, utc(2025, 1, 16), 180)) < 2.0, "Mars opposition 2025-01-16");
    check(std::fabs(crossing_day(Planet::Jupiter, utc(2026, 1, 10), 180)) < 2.0, "Jupiter opposition 2026-01-10");
    check(std::fabs(crossing_day(Planet::Saturn, utc(2025, 9, 21), 180)) < 2.0, "Saturn opposition 2025-09-21");
    // Uranus and Neptune oppositions from the almanac (2025-11-21, 2025-09-23, 2026-11-25).
    check(std::fabs(crossing_day(Planet::Uranus, utc(2025, 11, 21), 180)) < 2.0, "Uranus opposition 2025-11-21");
    check(std::fabs(crossing_day(Planet::Neptune, utc(2025, 9, 23), 180)) < 2.0, "Neptune opposition 2025-09-23");
    check(std::fabs(crossing_day(Planet::Uranus, utc(2026, 11, 25), 180)) < 2.0, "Uranus opposition 2026-11-25");
    check(std::fabs(planet_position(Planet::Uranus, {utc(2025, 11, 21)}).distance_au - 18.6) < 0.4, "Uranus distance at opposition (about 18.6 AU)");
    check(std::fabs(planet_position(Planet::Neptune, {utc(2025, 9, 23)}).distance_au - 28.9) < 0.3, "Neptune distance at opposition");
    check(std::fabs(crossing_day(Planet::Mars, utc(2027, 2, 19), 180)) < 2.0, "Mars opposition 2027-02-19");
    // Venus inferior conjunction 2025-03-23.
    check(std::fabs(crossing_day(Planet::Venus, utc(2025, 3, 23), 0)) < 2.0, "Venus inferior conjunction 2025-03-23");
    // Venus greatest western elongation 2025-06-01: 45.9 degrees.
    check(std::fabs(planet_position(Planet::Venus, {utc(2025, 6, 1)}).elongation_deg - 45.9) < 0.5, "Venus 45.9 deg on 2025-06-01");
    // Closest Mars approach of the 2025 opposition: about 0.642 AU.
    check(std::fabs(planet_position(Planet::Mars, {utc(2025, 1, 12)}).distance_au - 0.642) < 0.01, "Mars distance Jan 2025");
    // Jupiter's distance at opposition is near 4.2 AU; Saturn's near 8.6 AU.
    const double dj = planet_position(Planet::Jupiter, {utc(2026, 1, 10)}).distance_au;
    check(dj > 4.1 && dj < 4.3, "Jupiter opposition distance");
    const double ds = planet_position(Planet::Saturn, {utc(2025, 9, 21)}).distance_au;
    check(ds > 8.5 && ds < 8.8, "Saturn opposition distance");
    // Mercury never strays more than 28 degrees from the Sun, Venus 47.5.
    double maxm = 0, maxv = 0;
    for (std::int64_t t = utc(2025, 1, 1); t < utc(2026, 1, 1); t += 6 * 3600) {
        maxm = std::fmax(maxm, planet_position(Planet::Mercury, {t}).elongation_deg);
        maxv = std::fmax(maxv, planet_position(Planet::Venus, {t}).elongation_deg);
    }
    check(maxm > 17 && maxm < 28.5, "Mercury elongation range");
    check(maxv > 40 && maxv < 47.8, "Venus elongation range");
    // Venus greatest eastern elongation 2025-01-10 was 47.2 degrees.
    check(std::fabs(planet_position(Planet::Venus, {utc(2025, 1, 10)}).elongation_deg - 47.2) < 0.5, "Venus 47.2 deg on 2025-01-10");

    // Geometry: at an opposition planet transits near local midnight, so at that
    // time it is high from a station on the planet's declination latitude.
    const PlanetPosition mars = planet_position(Planet::Mars, {utc(2025, 1, 16)});
    check(mars.ra_deg > 100 && mars.ra_deg < 130 && mars.dec_deg > 20 && mars.dec_deg < 28, "Mars in Gemini at opposition");
    const PlanetSubpoint sp = planet_subpoint(Planet::Mars, {utc(2025, 1, 16, 5)});
    check(std::fabs(sp.latitude_deg - mars.dec_deg) < 0.5, "subpoint latitude is declination");
    const PlanetHorizontal h = planet_horizontal(Planet::Mars, {utc(2025, 1, 16, 5)}, sp.latitude_deg, sp.longitude_deg);
    check(h.elevation_deg > 89.0, "planet is overhead at its subpoint");
    bool threw = false;
    try { planet_horizontal(Planet::Mars, {0}, 91, 0); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "bad latitude throws");

    // Rows: only planets above the horizon, highest first; layer has all five.
    const QVariantList rows = planet_rows({utc(2025, 1, 16, 5)}, 51.5, -0.1);
    bool sorted = true;
    double prev = 91;
    for (const QVariant& r : rows) {
        const QString n = r.toMap().value("name").toString();
        for (int i = 0; i < kPlanetCount; ++i)
            if (n == planet_name(static_cast<Planet>(i))) {
                const double el = planet_horizontal(static_cast<Planet>(i), {utc(2025, 1, 16, 5)}, 51.5, -0.1).elevation_deg;
                if (el <= 0 || el > prev) sorted = false;
                prev = el;
            }
    }
    check(sorted, "rows above horizon, highest first");
    bool mars_row = false;
    for (const QVariant& r : rows) mars_row |= r.toMap().value("name").toString() == "Mars";
    check(mars_row, "Mars is up from London at 05:00Z on opposition night");
    const OverlayLayer layer = planet_overlay_layer({utc(2025, 1, 16, 5)});
    check(layer.id == "planets" && layer.markers.size() == 7, "overlay layer has seven planets");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
