// Satellite pass alerts: preferences (persistence, selection, quiet hours) and
// the scheduling plan (min elevation, lead time, ordering, the 64-notification
// budget, stable ids, cancel on disable, in-app banner). Pure, offline.
#include "../sat_alert_plan.hpp"
#include "../sat_alert_chime.hpp"
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <iostream>

namespace {
using namespace hamclock;
int checks = 0, failures = 0;
void check(bool ok, const std::string& label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}

constexpr int kIss = 25544, kSo50 = 27607, kAo27 = 22825, kOther = 99999;

SatCatalog test_catalog() {
    const QByteArray json = R"json({"updated":"t","satellites":[
      {"norad":25544,"name":"ISS","tle_name":"ISS (ZARYA)","status":"active",
       "transponders":[{"mode":"FM","uplink_mhz":145.99,"downlink_mhz":437.8,"ctcss_hz":67}]},
      {"norad":27607,"name":"SO-50","tle_name":"SAUDISAT 1C (SO-50)","status":"active",
       "transponders":[{"mode":"FM","uplink_mhz":145.85,"downlink_mhz":436.795,"ctcss_hz":67}]},
      {"norad":22825,"name":"AO-27","tle_name":"EYESAT A (AO-27)","status":"inactive",
       "transponders":[{"mode":"FM","uplink_mhz":145.85,"downlink_mhz":436.795}]}]})json";
    return parse_sat_catalog(json).catalog;
}

const qint64 kNow = 1793275200; // 2026-10-29 12:00 UTC

AlertPass pass(int norad, const char* name, qint64 aos, double max_el, double sun_el = -10) {
    AlertPass p;
    p.norad = norad;
    p.name = name;
    p.aos = aos;
    p.tca = aos + 300;
    p.los = aos + 600;
    p.max_el_deg = max_el;
    p.aos_compass = "NW";
    p.los_compass = "SE";
    p.sun_el_deg = sun_el;
    return p;
}

SatAlertPrefs on() {
    SatAlertPrefs p;
    p.enabled = true;
    return p;
}

void test_selection(const SatCatalog& cat) {
    SatAlertPrefs p = on();
    check(is_alert_selected(p, cat, kIss) && is_alert_selected(p, cat, kSo50), "all workable selects active birds");
    check(!is_alert_selected(p, cat, kAo27), "inactive bird not in all workable");
    check(!is_alert_selected(p, cat, kOther), "non-catalog bird not in all workable");
    const SatAlertPrefs muted = toggle_alert_satellite(p, cat, kSo50);
    check(!is_alert_selected(muted, cat, kSo50) && is_alert_selected(muted, cat, kIss), "disable one bird");
    check(is_alert_selected(p, cat, kSo50), "toggle returns a copy (original unchanged)");
    check(is_alert_selected(toggle_alert_satellite(muted, cat, kSo50), cat, kSo50), "re-enable a muted bird");
    check(toggle_alert_satellite(muted, cat, kSo50).muted.isEmpty(), "re-enable clears the mute");
    const SatAlertPrefs other = toggle_alert_satellite(p, cat, kOther);
    check(is_alert_selected(other, cat, kOther), "pick a non-catalog bird");
    check(!is_alert_selected(toggle_alert_satellite(other, cat, kOther), cat, kOther), "unpick it");
    const SatAlertPrefs custom = with_all_workable(other, false);
    check(!is_alert_selected(custom, cat, kIss) && is_alert_selected(custom, cat, kOther), "custom keeps picks only");
    check(is_alert_selected(toggle_alert_satellite(custom, cat, kIss), cat, kIss), "custom: pick a catalog bird");
    check(is_alert_selected(toggle_alert_satellite(custom, cat, kAo27), cat, kAo27), "inactive bird can be picked");
    check(!is_alert_selected(toggle_alert_satellite(p, cat, -3), cat, -3), "bad ids ignored");
}

void test_quiet_hours() {
    SatAlertPrefs p = on();
    check(!in_quiet_hours(p, QTime(3, 0)), "no quiet hours by default");
    p.quiet_start_hour = 22;
    p.quiet_end_hour = 7;
    check(in_quiet_hours(p, QTime(23, 30)) && in_quiet_hours(p, QTime(6, 59)), "quiet window wraps midnight");
    check(!in_quiet_hours(p, QTime(7, 0)) && !in_quiet_hours(p, QTime(21, 59)), "outside the window");
    p.quiet_start_hour = 1;
    p.quiet_end_hour = 5;
    check(in_quiet_hours(p, QTime(1, 0)) && !in_quiet_hours(p, QTime(5, 0)), "same-day window");
}

void test_persistence() {
    QTemporaryDir dir;
    const QString path = dir.filePath("options.ini");
    {
        QSettings ini(path, QSettings::IniFormat);
        check(load_sat_alert_prefs(ini) == SatAlertPrefs{}, "defaults when nothing saved");
        SatAlertPrefs p = on();
        p.all_workable = false;
        p.picked = {kOther, kIss};
        p.muted = {kSo50};
        p.min_elevation_deg = 30;
        p.lead_minutes = 10;
        p.quiet_start_hour = 22;
        p.quiet_end_hour = 6;
        p.daylight_only = true;
        p.sound = false;
        p.announce_start = false;
        save_sat_alert_prefs(ini, p);
    }
    QSettings again(path, QSettings::IniFormat);
    const SatAlertPrefs back = load_sat_alert_prefs(again);
    check(back.enabled && !back.all_workable && back.min_elevation_deg == 30 && back.lead_minutes == 10 &&
          back.quiet_start_hour == 22 && back.quiet_end_hour == 6 && back.daylight_only && !back.sound &&
          !back.announce_start, "settings round trip");
    check(back.picked == QList<int>({kIss, kOther}) && back.muted == QList<int>({kSo50}), "ids round trip, sorted");
    again.setValue("satalerts/minElevation", 400);
    again.setValue("satalerts/leadMinutes", -9);
    again.setValue("satalerts/quietStart", 30);
    again.setValue("satalerts/picked", "12,abc,-4,12");
    const SatAlertPrefs clamped = load_sat_alert_prefs(again);
    check(clamped.min_elevation_deg == 90 && clamped.lead_minutes == 0, "out-of-range values clamped");
    check(clamped.quiet_start_hour == -1 && clamped.quiet_end_hour == -1, "half a quiet window is none");
    check(clamped.picked == QList<int>({12}), "junk ids dropped");
    again.setValue("satalerts/minElevation", "lots");
    again.setValue("satalerts/quietStart", "abc");
    again.setValue("satalerts/quietEnd", 7);
    again.setValue("satalerts/sound", "maybe");
    const SatAlertPrefs junk = load_sat_alert_prefs(again);
    check(junk.min_elevation_deg == kDefaultAlertMinElevation && junk.quiet_start_hour == -1 && junk.sound,
          "non-numeric values fall back to defaults, not 0");
}

void test_filtering(const SatCatalog& cat) {
    const QTimeZone utc = QTimeZone::utc();
    SatAlertPrefs p = on();
    const std::vector<AlertPass> passes = {
        pass(kIss, "ISS", kNow + 3600, 54), pass(kSo50, "SO-50", kNow + 1800, 12),
        pass(kAo27, "AO-27", kNow + 600, 80), pass(kIss, "ISS", kNow - 2000, 40)};
    const auto kept = alert_passes(passes, p, cat, utc, kNow);
    check(kept.size() == 1 && kept[0].norad == kIss && kept[0].aos == kNow + 3600,
          "selected, high enough, not over");
    p.min_elevation_deg = 10;
    check(alert_passes(passes, p, cat, utc, kNow).size() == 2, "lower minimum lets SO-50 in");
    p.quiet_start_hour = 12;
    p.quiet_end_hour = 14;
    check(alert_passes(passes, p, cat, utc, kNow).empty(), "quiet hours by AOS local time");
    check(alert_passes(passes, p, cat, QTimeZone("America/New_York"), kNow).size() == 2, "quiet hours use the zone");
    SatAlertPrefs day = on();
    day.daylight_only = true;
    check(alert_passes({pass(kIss, "ISS", kNow + 3600, 54, 20)}, day, cat, utc, kNow).size() == 1 &&
          alert_passes({pass(kIss, "ISS", kNow + 3600, 54, -3)}, day, cat, utc, kNow).empty(), "daylight only");
}

void test_plan(const SatCatalog& cat) {
    const QTimeZone utc = QTimeZone::utc();
    SatAlertPrefs p = on();
    p.lead_minutes = 10;
    const auto plan = plan_alerts({pass(kSo50, "SO-50", kNow + 7200, 30), pass(kIss, "ISS", kNow + 3600, 54)},
                                  p, cat, utc, kNow);
    check(plan.size() == 2 && plan[0].norad == kIss && plan[1].norad == kSo50, "nearest first");
    check(plan[0].fire == kNow + 3000, "fires lead minutes before AOS");
    check(plan[0].id == alert_id(kIss, kNow + 3600) && plan[0].id.startsWith(kSatAlertIdPrefix), "stable prefixed id");
    check(alert_id(kIss, kNow + 3600) == alert_id(kIss, kNow + 3600 + 20), "AOS jitter within a minute keeps the id");
    check(alert_id(kIss, kNow + 3600) != alert_id(kSo50, kNow + 3600), "id differs per satellite");
    check(plan[0].title == "ISS pass in 10 min", "title");
    check(plan[0].body == "AOS 13:00 · max 54° · NW → SE · FM 437.800 MHz down, 145.990 up, 67.0 Hz", "body text");
    check(plan_alerts({pass(kIss, "ISS", kNow + 300, 54)}, p, cat, utc, kNow).empty(), "lead already elapsed");
    p.lead_minutes = 0;
    check(plan_alerts({pass(kIss, "ISS", kNow + 300, 54)}, p, cat, utc, kNow).size() == 1, "lead 0 fires at AOS");
    check(plan_alerts({pass(kIss, "ISS", kNow + 300, 54)}, p, cat, utc, kNow)[0].title == "ISS is rising now",
          "lead 0 title");
    const auto dup = plan_alerts({pass(kIss, "ISS", kNow + 3600, 54), pass(kIss, "ISS", kNow + 3610, 54)}, p, cat, utc, kNow);
    check(dup.size() == 1, "duplicate passes deduplicated by id");

    std::vector<AlertPass> many;
    for (int i = 200; i > 0; --i) many.push_back(pass(i % 2 ? kIss : kSo50, i % 2 ? "ISS" : "SO-50", kNow + i * 900, 45));
    const auto capped = plan_alerts(many, p, cat, utc, kNow);
    check(static_cast<int>(capped.size()) == kMaxScheduledAlerts, "capped at the notification budget");
    check(kMaxScheduledAlerts < kAppleMaxPendingNotifications, "budget leaves room under the iOS limit");
    bool ordered = true;
    for (std::size_t i = 1; i < capped.size(); ++i) ordered = ordered && capped[i - 1].fire <= capped[i].fire;
    check(ordered && capped.front().aos == kNow + 900 && capped.back().aos == kNow + 60 * 900, "keeps the nearest");
    check(plan_alerts(many, p, cat, utc, kNow, 5).size() == 5, "custom cap");

    SatAlertPrefs off = p;
    off.enabled = false;
    check(plan_alerts(many, off, cat, utc, kNow).empty(), "alerts off: empty plan");
    const QStringList pending = {alert_id(kIss, kNow + 900), "pass.ISS.123", alert_id(kSo50, 1)};
    check(stale_alert_ids(pending, {}) == QStringList({alert_id(kIss, kNow + 900), alert_id(kSo50, 1)}),
          "disable cancels every alert but not other reminders");
    check(stale_alert_ids(pending, capped) == QStringList({alert_id(kSo50, 1)}), "only dropped alerts cancelled");
    const SatAlertPrefs deselected = toggle_alert_satellite(p, cat, kSo50);
    const auto iss_only = plan_alerts(many, deselected, cat, utc, kNow);
    bool none = true;
    for (const auto& a : iss_only) none = none && a.norad == kIss;
    check(none && !iss_only.empty(), "deselected bird leaves the plan");
    check(stale_alert_ids({alert_id(kSo50, kNow + 1800)}, iss_only).size() == 1, "deselected bird cancelled");
}

void test_banner(const SatCatalog& cat) {
    const QTimeZone utc = QTimeZone::utc();
    SatAlertPrefs p = on();
    const std::vector<AlertPass> passes = {pass(kIss, "ISS", kNow + 240, 54), pass(kSo50, "SO-50", kNow + 3600, 40)};
    const auto soon = current_banner(passes, p, cat, utc, kNow, {});
    check(soon && !soon->live && soon->text.startsWith("ISS in 4 min") && soon->id.endsWith(".soon"), "upcoming banner");
    check(!current_banner(passes, p, cat, utc, kNow - 600, {}), "nothing before lead time");
    const auto up = current_banner(passes, p, cat, utc, kNow + 300, {});
    check(up && up->live && up->text.startsWith("ISS is up now") && up->id.endsWith(".up"), "in-progress banner");
    check(!current_banner(passes, p, cat, utc, kNow, {soon->id}), "dismissed banner stays hidden");
    check(current_banner(passes, p, cat, utc, kNow + 300, {soon->id}).has_value(), "AOS banner after dismissing soon");
    SatAlertPrefs quiet_start = p;
    quiet_start.announce_start = false;
    check(!current_banner(passes, quiet_start, cat, utc, kNow + 300, {}), "no AOS banner when turned off");
    SatAlertPrefs off = p;
    off.enabled = false;
    check(!current_banner(passes, off, cat, utc, kNow, {}), "no banner when alerts are off");
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const SatCatalog cat = test_catalog();
    check(cat.entries.size() == 3, "test catalog");
    test_selection(cat);
    test_quiet_hours();
    test_persistence();
    test_filtering(cat);
    test_plan(cat);
    test_banner(cat);
    const QByteArray wav = alert_chime_wav();
    check(wav.startsWith("RIFF") && wav.mid(8, 4) == "WAVE" && wav.size() > 10000, "chime is a WAV");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
