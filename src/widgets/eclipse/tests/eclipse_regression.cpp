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

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
