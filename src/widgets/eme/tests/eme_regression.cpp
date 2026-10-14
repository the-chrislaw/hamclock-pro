// EME planning: path loss reference and mutual-window logic, checked against
// the lunar ephemeris itself (edges are zero crossings of the elevation).
#include "../eme_model.hpp"
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
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;

    check(eme_degradation_db(kMoonPerigeeKm) == 0.0, "perigee has no degradation");
    check(eme_degradation_db(300000.0) == 0.0, "closer than perigee clamps to 0");
    // Apogee/perigee ratio 1.1411 -> 40 log10 = 2.3 dB (the usual quoted figure).
    check(std::fabs(eme_degradation_db(kMoonApogeeKm) - 2.3) < 0.05, "apogee is about 2.3 dB worse");

    // Moon within a few degrees of the same sky position for two nearby stations:
    // the mutual window is almost the whole visible pass and must bracket "up" times.
    const double la1 = 51.5, lo1 = -0.1, la2 = 48.9, lo2 = 2.3; // London / Paris
    const UtcSeconds t0{utc(2025, 3, 14, 0, 0)};
    const auto w = next_mutual_moon_window(t0, la1, lo1, la2, lo2);
    check(w.has_value(), "London-Paris window exists");
    if (w) {
        auto up = [&](std::int64_t t) {
            return moon_horizontal({t}, la1, lo1).elevation_deg > 0 && moon_horizontal({t}, la2, lo2).elevation_deg > 0;
        };
        check(w->end.value > w->start.value, "window has positive length");
        check(up((w->start.value + w->end.value) / 2), "both up mid-window");
        check(up(w->start.value) && (w->in_progress || !up(w->start.value - 2)), "start is the up edge");
        check(up(w->end.value - 1) && !up(w->end.value + 1), "end is the down edge");
        // Full moon 2025-03-14: it is up at midnight, so the window is already running.
        check(w->in_progress == up(t0.value), "in_progress matches the state at the search start");
    }

    // Antipodal stations never see the Moon together.
    check(!next_mutual_moon_window(t0, 40.0, 10.0, -40.0, -170.0).has_value(), "antipodes: no window");

    // In-progress: start the search in the middle of the window found above.
    if (w) {
        const auto mid = next_mutual_moon_window({(w->start.value + w->end.value) / 2}, la1, lo1, la2, lo2);
        check(mid && mid->in_progress && std::llabs(mid->end.value - w->end.value) <= 1, "in-progress window ends at the same edge");
    }

    const EmeSummary s = summarize_eme(t0, la1, lo1, -33.9, 151.2);
    check(s.distance.endsWith(" km") && s.degradation.startsWith("+") && s.de_position.startsWith("Az "), "summary text shape");
    check(!s.window.isEmpty(), "summary has a window text");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
