// Sunrise/sunset against published almanac values and the Meeus-tested
// solar_position module. Offline; no fixtures needed.
#include "../sun_moon_model.hpp"
#include "../../../domain/solar_position.hpp"
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

    // London (51.5074 N, 0.1278 W), 2026-06-21: timeanddate/HM Nautical
    // Almanac give sunrise 04:43 BST and sunset 21:21 BST = 03:43 / 20:21 UTC.
    auto london = sun_events_for_utc_day(utc(2026, 6, 21), 51.5074, -0.1278);
    check(london.events.size() == 2 && london.events[0].kind == SunEventKind::Rise, "London has rise then set");
    if (london.events.size() == 2) {
        check(std::llabs(london.events[0].time.value - utc(2026, 6, 21, 3, 43)) <= 120, "London sunrise 03:43 UTC +-2 min");
        check(std::llabs(london.events[1].time.value - utc(2026, 6, 21, 20, 21)) <= 120, "London sunset 20:21 UTC +-2 min");
    }
    // London 2026-12-21: sunrise 08:04 GMT, sunset 15:53 GMT.
    auto winter = sun_events_for_utc_day(utc(2026, 12, 21), 51.5074, -0.1278);
    check(winter.events.size() == 2 &&
          std::llabs(winter.events[0].time.value - utc(2026, 12, 21, 8, 4)) <= 120 &&
          std::llabs(winter.events[1].time.value - utc(2026, 12, 21, 15, 53)) <= 120, "London winter solstice times");

    // Self-consistency with the Meeus-tested position: elevation at each event is -0.833 deg.
    for (double lat : {-33.87, 0.0, 35.68, 51.5, 64.0}) {
        for (double lon : {-122.4, 0.0, 151.2}) {
            const auto day = sun_events_for_utc_day(utc(2026, 3, 20), lat, lon);
            for (const auto& e : day.events) {
                const double el = solar_elevation_deg(subsolar_point(e.time), lat, lon);
                check(std::fabs(el + 0.833) < 0.12, "elevation at event is horizon (-0.833 deg)");
            }
            check(day.events.size() == 2, "two events at equinox");
        }
    }
    // Events near the date line fall inside the requested UTC day (Auckland is UTC+13 in Dec).
    auto akl = sun_events_for_utc_day(utc(2026, 12, 20), -36.85, 174.76);
    bool inside = true;
    for (const auto& e : akl.events) inside = inside && e.time.value >= utc(2026, 12, 20) && e.time.value < utc(2026, 12, 21);
    check(inside && !akl.events.empty(), "events lie within the UTC day");

    // Polar cases: Tromso (69.65 N, 18.96 E).
    auto summer = sun_events_for_utc_day(utc(2026, 6, 21), 69.65, 18.96);
    check(summer.events.empty() && summer.polar == PolarState::MidnightSun, "Tromso midnight sun");
    auto dark = sun_events_for_utc_day(utc(2026, 12, 21), 69.65, 18.96);
    check(dark.events.empty() && dark.polar == PolarState::PolarNight, "Tromso polar night");
    check(!next_sun_event({utc(2026, 6, 21)}, SunEventKind::Set, 69.65, 18.96), "no sunset in midnight sun");

    // next_sun_event crosses midnight.
    const auto next = next_sun_event({utc(2026, 6, 21, 21, 0)}, SunEventKind::Rise, 51.5074, -0.1278);
    check(next && std::llabs(next->value - utc(2026, 6, 22, 3, 43)) <= 120, "next sunrise is tomorrow");

    // Hostile input.
    bool threw = false;
    try { sun_events_for_utc_day(0, 91, 0); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "latitude 91 rejected");
    threw = false;
    try { sun_events_for_utc_day(0, 0, std::nan("")); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "NaN longitude rejected");

    // Summary text.
    const auto s = summarize_sun({utc(2026, 6, 21, 12, 0)}, 51.5074, -0.1278);
    check(s.next_set == "20:21 UTC" || s.next_set == "20:20 UTC" || s.next_set == "20:22 UTC", "summary next sunset text");
    check(s.day_length.startsWith("16 h"), "London June day length about 16 h 38");
    check(summarize_sun({utc(2026, 6, 21, 12, 0)}, 69.65, 18.96).note == "Midnight sun", "summary note");

    // Model without settings is unconfigured, never invents times.
    WidgetContext context;
    SunMoonModel model(context);
    model.recompute();
    check(model.state() == "unconfigured" && !model.hasStation() && model.deRise().isEmpty(), "unconfigured without station");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
