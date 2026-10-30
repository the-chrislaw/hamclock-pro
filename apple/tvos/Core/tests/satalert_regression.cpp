// Satellite pass alert port; mirrors src/widgets/satellites/tests/sat_alert_plan_regression and
// sat_catalog_regression (the oracles): selection, quiet hours, filtering, body text, banner, plus
// the shipped catalog and a real prediction through the C ABI.
#include "../hcpcore.h"
#include "../satalert.hpp"
#include "test_util.hpp"
#include <string>
#include <vector>

using namespace hcptest;
using namespace hcp::satalert;

namespace {
constexpr int kIss = 25544, kSo50 = 27607, kAo27 = 22825, kOther = 99999;
constexpr int64_t kNow = 1793275200; // 2026-10-29 12:00 UTC

const char* kCatalogJson = R"json({"updated":"t","satellites":[
  {"norad":25544,"name":"ISS","tle_name":"ISS (ZARYA)","status":"active",
   "transponders":[{"mode":"FM","uplink_mhz":145.99,"downlink_mhz":437.8,"ctcss_hz":67}]},
  {"norad":27607,"name":"SO-50","tle_name":"SAUDISAT 1C (SO-50)","status":"active",
   "transponders":[{"mode":"FM","uplink_mhz":145.85,"downlink_mhz":436.795,"ctcss_hz":67}]},
  {"norad":22825,"name":"AO-27","tle_name":"EYESAT A (AO-27)","status":"inactive",
   "transponders":[{"mode":"FM","uplink_mhz":145.85,"downlink_mhz":436.795}]}]})json";

AlertPass pass(int norad, const char* name, int64_t aos, double max_el, double sun_el = -10) {
    return AlertPass{norad, name, aos, aos + 300, aos + 600, max_el, sun_el, "NW", "SE"};
}

Prefs on() {
    Prefs p;
    p.enabled = true;
    return p;
}
} // namespace

int main() {
    const Catalog cat = parse_catalog(kCatalogJson);
    check(cat.entries.size() == 3, "test catalog");

    // Selection (is_alert_selected / toggle_alert_satellite).
    const Prefs p = on();
    check(is_selected(p, cat, kIss) && is_selected(p, cat, kSo50), "all workable selects active birds");
    check(!is_selected(p, cat, kAo27) && !is_selected(p, cat, kOther), "inactive and non-catalog birds not in all workable");
    const Prefs muted = toggle(p, cat, kSo50);
    check(!is_selected(muted, cat, kSo50) && is_selected(muted, cat, kIss), "disable one bird");
    check(is_selected(toggle(muted, cat, kSo50), cat, kSo50) && toggle(muted, cat, kSo50).muted.empty(),
          "re-enable clears the mute");
    const Prefs other = toggle(p, cat, kOther);
    check(is_selected(other, cat, kOther) && !is_selected(toggle(other, cat, kOther), cat, kOther), "pick and unpick a non-catalog bird");
    Prefs custom = other;
    custom.all_workable = false;
    custom.muted.clear();
    check(!is_selected(custom, cat, kIss) && is_selected(custom, cat, kOther), "custom keeps picks only");
    check(is_selected(toggle(custom, cat, kAo27), cat, kAo27), "inactive bird can be picked");
    check(!is_selected(toggle(p, cat, -3), cat, -3), "bad ids ignored");

    // Quiet hours.
    Prefs q = on();
    check(!in_quiet_hours(q, 3), "no quiet hours by default");
    q.quiet_start_hour = 22;
    q.quiet_end_hour = 7;
    check(in_quiet_hours(q, 23) && in_quiet_hours(q, 6) && !in_quiet_hours(q, 7) && !in_quiet_hours(q, 21), "window wraps midnight");
    q.quiet_start_hour = 0;
    q.quiet_end_hour = 3;
    check(in_quiet_hours(q, 1) && !in_quiet_hours(q, 5), "same-day window");
    Prefs bad;
    bad.min_elevation_deg = 120;
    bad.lead_minutes = -4;
    bad.quiet_start_hour = 5;
    bad.picked = {12, -1, 12, 0};
    const Prefs clamped = sanitized(bad);
    check(clamped.min_elevation_deg == 90 && clamped.lead_minutes == 0 && clamped.quiet_start_hour == -1 &&
              clamped.picked == std::vector<int>{12},
          "sanitized");

    // Filtering.
    Prefs f = on();
    const std::vector<AlertPass> passes = {pass(kIss, "ISS", kNow + 3600, 54), pass(kSo50, "SO-50", kNow + 1800, 12),
                                           pass(kAo27, "AO-27", kNow + 600, 80), pass(kIss, "ISS", kNow - 2000, 40)};
    const auto kept = alert_passes(passes, f, cat, 0, kNow);
    check(kept.size() == 1 && kept[0].norad == kIss && kept[0].aos == kNow + 3600, "selected, high enough, not over");
    f.min_elevation_deg = 10;
    check(alert_passes(passes, f, cat, 0, kNow).size() == 2, "lower minimum lets SO-50 in");
    f.quiet_start_hour = 12;
    f.quiet_end_hour = 14;
    check(alert_passes(passes, f, cat, 0, kNow).empty(), "quiet hours by AOS local time");
    check(alert_passes(passes, f, cat, -4 * 3600, kNow).size() == 2, "quiet hours use the zone (EDT)");
    Prefs day = on();
    day.daylight_only = true;
    check(alert_passes({pass(kIss, "ISS", kNow + 3600, 54, 20)}, day, cat, 0, kNow).size() == 1 &&
              alert_passes({pass(kIss, "ISS", kNow + 3600, 54, -3)}, day, cat, 0, kNow).empty(),
          "daylight only");
    check(alert_body(pass(kIss, "ISS", kNow + 3600, 54), cat.find(kIss), 0) ==
              "AOS 13:00 · max 54° · NW → SE · FM 437.800 MHz down, 145.990 up, 67.0 Hz",
          "body text");

    // Banner.
    const Prefs b = on();
    const std::vector<AlertPass> two = {pass(kIss, "ISS", kNow + 240, 54), pass(kSo50, "SO-50", kNow + 3600, 40)};
    const auto soon = current_banner(two, b, cat, 0, kNow);
    check(soon && !soon->live && soon->text.rfind("ISS in 4 min", 0) == 0, "upcoming banner");
    check(!current_banner(two, b, cat, 0, kNow - 600), "nothing before lead time");
    const auto up = current_banner(two, b, cat, 0, kNow + 300);
    check(up && up->live && up->text.rfind("ISS is up now", 0) == 0, "in-progress banner");
    Prefs no_start = b;
    no_start.announce_start = false;
    check(!current_banner(two, no_start, cat, 0, kNow + 300), "no AOS banner when turned off");
    Prefs off = b;
    off.enabled = false;
    check(!current_banner(two, off, cat, 0, kNow), "no banner when alerts are off");

    // The catalog the apps ship, and the transponder text (sat_catalog_regression).
    const auto shipped = read_file(std::string(HCP_WIDGET_FIXTURES) + "/satellites/amateur-satellites.json");
    const Catalog real = parse_catalog(shipped);
    check(real.entries.size() >= 15 && real.find(kIss) && real.find(kIss)->name == "ISS", "shipped catalog parses");
    check(describe({"APRS", {145.825, 145.825}, {145.825, 145.825}, 0}) == "APRS 145.825 MHz", "simplex text");
    check(describe({"Linear", {145.9, 145.96}, {435.61, 435.67}, 0}) == "Linear 435.610-435.670 MHz down, 145.900-145.960 up",
          "linear passband text");
    check(parse_catalog("[]").entries.empty() && parse_catalog("{\"satellites\":[{\"norad\":1,\"name\":\"X\",\"status\":\"active\","
                                                               "\"transponders\":[{\"mode\":\"CW\",\"downlink_mhz\":1}]}]}")
                                                     .entries.empty(),
          "bad catalog and unknown mode skipped");

    // C ABI with a real prediction on the CelesTrak amateur fixture (ISS selected at FN31pr).
    const auto tle = read_file(std::string(HCP_WIDGET_FIXTURES) + "/../../tests/fixtures/sources/celestrak-amateur.tle");
    HcpSatAlertPrefs ap{1, 1, 0, 10, -1, -1, 0, 1, nullptr, 0, nullptr, 0};
    std::vector<HcpAlertPass> out(400);
    const int64_t when = 1792000000; // 2026-10-15
    const int n = hcp_satalert_predict(tle.data(), tle.size(), shipped.data(), shipped.size(), &ap, 41.71, -72.73, when,
                                       out.data(), out.size());
    bool sorted = n > 0, has_iss = false;
    for (int i = 1; i < n; ++i) sorted = sorted && out[static_cast<size_t>(i - 1)].aos_s <= out[static_cast<size_t>(i)].aos_s;
    for (int i = 0; i < n; ++i) has_iss = has_iss || out[static_cast<size_t>(i)].norad == kIss;
    check(n > 10 && sorted && has_iss, "48 h of passes for the selected birds, sorted, ISS among them");
    char text[256];
    int live = 0;
    bool any = false;
    for (int64_t t = when; t < when + 86400 && !any; t += 60)
        any = hcp_satalert_banner(out.data(), static_cast<size_t>(n), &ap, shipped.data(), shipped.size(), 0, t, text,
                                  sizeof text, &live) == 1;
    check(any && std::string(text).find(" · AOS ") != std::string::npos, "banner fires within a day");
    HcpSatCatalogEntry entries[64];
    const int ne = hcp_satalert_catalog(shipped.data(), shipped.size(), entries, 64);
    check(ne == static_cast<int>(real.entries.size()) && hcp_satalert_selected(&ap, shipped.data(), shipped.size(), kIss) == 1,
          "C ABI catalog and selection");
    return finish("satalert");
}
