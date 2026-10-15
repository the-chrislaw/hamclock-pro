// Solar eclipse circumstances checked against published NASA/Espenak catalogue
// values (time of greatest eclipse UTC, gamma).
#include "../eclipse_model.hpp"
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
std::int64_t utc(int y, int mo, int d, int h = 0, int mi = 0, int s = 0) {
    return QDateTime(QDate(y, mo, d), QTime(h, mi, s), QTimeZone::utc()).toSecsSinceEpoch();
}
struct Known { std::int64_t before, greatest; double gamma; hamclock::SolarEclipseType type; const char* label; };
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    using T = SolarEclipseType;
    const Known known[] = {
        {utc(2023, 4, 1), utc(2023, 4, 20, 4, 17, 56), -0.3952, T::Hybrid, "2023-04-20 hybrid"},
        {utc(2023, 9, 1), utc(2023, 10, 14, 18, 0, 41), 0.3753, T::Annular, "2023-10-14 annular"},
        {utc(2024, 1, 1), utc(2024, 4, 8, 18, 17, 16), 0.3431, T::Total, "2024-04-08 total"},
        {utc(2025, 1, 1), utc(2025, 3, 29, 10, 48, 36), 1.0405, T::Partial, "2025-03-29 partial"},
        {utc(2026, 1, 1), utc(2026, 2, 17, 12, 11, 52), -0.9743, T::Annular, "2026-02-17 annular"},
        {utc(2026, 3, 1), utc(2026, 8, 12, 17, 46, 6), 0.8977, T::Total, "2026-08-12 total"},
        {utc(2027, 1, 1), utc(2027, 2, 6, 15, 59, 49), -0.2952, T::Annular, "2027-02-06 annular"},
        {utc(2027, 3, 1), utc(2027, 8, 2, 10, 6, 52), 0.1421, T::Total, "2027-08-02 total"},
    };
    for (const auto& k : known) {
        const auto e = next_solar_eclipse(UtcSeconds{k.before});
        check(e.has_value(), k.label);
        if (!e) continue;
        const std::int64_t dt = e->greatest.value - k.greatest;
        if (std::llabs(dt) > 300 || std::fabs(e->gamma - k.gamma) > 0.01 || e->type != k.type)
            std::cerr << k.label << ": dt " << dt << " s, gamma " << e->gamma << " type " << solar_eclipse_type_name(e->type) << '\n';
        check(std::llabs(dt) <= 300, k.label);
        check(std::fabs(e->gamma - k.gamma) <= 0.01, k.label);
        check(e->type == k.type, k.label);
    }
    // "strictly after": asking at the instant of greatest eclipse returns the next one.
    const auto e24 = next_solar_eclipse(UtcSeconds{utc(2024, 4, 8, 18, 17, 16) - 600});
    const auto after = next_solar_eclipse(UtcSeconds{utc(2024, 4, 8, 19, 0, 0)});
    check(e24 && after && after->greatest.value > e24->greatest.value, "next is strictly later");

    const EclipseSummary s = summarize_eclipse(UtcSeconds{utc(2026, 8, 1)});
    check(s.kind == "Total solar eclipse", "summary kind");
    check(s.when.startsWith("12 Aug 2026 17:4"), "summary time");
    check(s.countdown == "in 11 d", "summary countdown");

    // Visibility from the 2024-04-08 eclipse (NASA path: Dallas total, Denver ~65% partial).
    const SolarEclipse e2024 = *next_solar_eclipse(UtcSeconds{utc(2024, 1, 1)});
    const auto dallas = eclipse_visibility(e2024, 32.78, -96.80);
    check(dallas.visibility == EclipseVisibility::Visible && dallas.min_separation_deg < 0.25, "Dallas sees it");
    check(dallas.sun_elevation_deg > 50 && dallas.sun_elevation_deg < 75, "Dallas sun high");
    check(eclipse_visibility(e2024, 39.74, -104.99).visibility == EclipseVisibility::Visible, "Denver sees partial");
    check(dallas.magnitude > 0.95, "Dallas magnitude near total");
    const auto denver = eclipse_visibility(e2024, 39.74, -104.99);
    check(denver.magnitude > 0.6 && denver.magnitude < 0.9, "Denver magnitude (NASA ~0.65 obscuration)");
    // Dallas totality about 18:40-18:44 UTC (NASA: 1:40-1:44 pm CDT), within 5 min.
    check(std::llabs(dallas.max_time.value - utc(2024, 4, 8, 18, 42, 0)) <= 300, "Dallas max time");
    // NASA Dallas: partial begins 17:23 UTC, ends 20:02 UTC (ephemeris-limited, allow 8 min).
    check(std::llabs(dallas.start_time.value - utc(2024, 4, 8, 17, 23, 0)) <= 480, "Dallas first contact");
    check(std::llabs(dallas.end_time.value - utc(2024, 4, 8, 20, 2, 0)) <= 480, "Dallas last contact");
    check(!dallas.start_at_sunrise && !dallas.end_at_sunset, "Dallas contacts not cut off by horizon");
    check(denver.start_time.value < denver.max_time.value && denver.max_time.value < denver.end_time.value, "Denver contacts bracket maximum");
    check(eclipse_visibility(e2024, 51.5, -0.13).start_time.value == 0, "London no contacts");
    check(eclipse_visibility(e2024, 51.5, -0.13).magnitude == 0.0, "London magnitude zero");
    check(eclipse_visibility(e2024, 51.5, -0.13).visibility == EclipseVisibility::None, "London: Sun well after");
    check(eclipse_visibility(e2024, -33.9, 151.2).visibility == EclipseVisibility::None, "Sydney: night");
    check(eclipse_visibility(e2024, 55.75, 37.6).visibility == EclipseVisibility::None, "Moscow: not visible");
    check(eclipse_visibility(e2024, 30.0, 31.2).visibility == EclipseVisibility::None, "Cairo: not visible");
    check(eclipse_visibility(e2024, 35.7, 139.7).visibility == EclipseVisibility::None, "Tokyo: night");
    bool threw = false;
    try { eclipse_visibility(e2024, 91, 0); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "invalid latitude throws");
    const EclipseSummary sd = summarize_eclipse(UtcSeconds{utc(2024, 1, 1)}, 32.78, -96.80);
    check(sd.visibility == "Visible at DE" && sd.local.startsWith("Max 18:") && sd.local.contains("Z"), "summary local line");
    check(sd.contacts.contains("17:") && sd.contacts.contains("19:"), "summary contacts line");
    const EclipseSummary sl = summarize_eclipse(UtcSeconds{utc(2024, 1, 1)}, 51.5, -0.13);
    check(sl.local.isEmpty(), "no local line when not visible");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
