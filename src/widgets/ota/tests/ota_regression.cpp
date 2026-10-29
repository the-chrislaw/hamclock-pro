// On-the-air widget: POTA/SOTA parsers and model, offline.
// Fixtures are trimmed real responses captured 2026-10-13 ~17:40 UTC.
#include "../ota_model.hpp"
#include "../ota_overlay.hpp"
#include "../ota_parse.hpp"
#include "../../../net/polled_source.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <algorithm>
#include <QTimeZone>
#include <QTimer>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
QByteArray fixture(const QString& dir, const char* name) {
    QFile file(QDir(dir).filePath(name));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    const QString dir = argv[1];
    const auto pota_body = fixture(dir, "activator.json");
    const auto sota_body = fixture(dir, "all.json");
    check(!pota_body.isEmpty() && !sota_body.isEmpty(), "fixtures present");

    const auto pota = ota::parse_pota(pota_body);
    // 8 rows, one "QRT" and one "Qrt" dropped.
    check(pota.ok && pota.spots.size() == 6, "POTA: 6 live spots (QRT dropped)");
    check(pota.spots.front().call == "K3AV" || pota.spots.front().call == "K5G", "POTA newest first");
    check(pota.spots.front().time == QDateTime(QDate(2026, 10, 13), QTime(17, 41, 38), QTimeZone::utc()), "POTA time is UTC");
    const auto& first = pota.spots.front();
    check(first.program == "POTA" && first.freq_khz > 0 && first.has_position, "POTA fields");

    const auto sota = ota::parse_sota(sota_body);
    check(sota.ok && sota.spots.size() == 8, "SOTA: 8 spots");
    check(sota.spots.front().call == "KQ4MHE" && sota.spots.front().reference == "W4C/CM-009", "SOTA newest");
    check(sota.spots.front().freq_khz == 14064.0, "SOTA MHz converted to kHz");
    check(ota::format_frequency(14064.0) == "14.064" && ota::format_frequency(144200) == "144.200", "frequency text");

    const auto merged = ota::merge(pota, sota);
    check(merged.size() == pota.spots.size() + sota.spots.size() - 2, "duplicate N7LKL/W7O/WV-005 collapsed");
    bool newest_first = true;
    for (size_t i = 1; i < merged.size(); ++i) newest_first = newest_first && merged[i - 1].time >= merged[i].time;
    check(newest_first, "merged newest first");

    const auto wwff = ota::parse_wwff(fixture(dir, "wwff.json"));
    check(wwff.ok && wwff.spots.size() == 7, "WWFF: 7 spots (QRT dropped)");
    check(wwff.spots.front().call == "VK3PF" && wwff.spots.front().reference == "VKFF-0753" &&
              wwff.spots.front().program == "WWFF" && wwff.spots.front().freq_khz == 7165.0 &&
              wwff.spots.front().name == "Bunyip State Park" && wwff.spots.front().has_position,
          "WWFF fields");
    check(wwff.spots.front().time == QDateTime(QDate(2026, 10, 17), QTime(8, 17, 1), QTimeZone::utc()), "WWFF epoch is UTC");
    check(!ota::parse_wwff("{}").ok && ota::parse_wwff("[{\"activator\":\"A1B\",\"reference\":\"KFF-1\"}]").spots.empty(),
          "WWFF malformed rows skipped");
    const auto all3 = ota::merge(pota, sota, wwff);
    check(all3.size() == merged.size() + wwff.spots.size(), "three feeds merged");
    {
        ota::Filter wf;
        wf.program = "WWFF";
        check(ota::filter(all3, wf, all3.front().time).size() == wwff.spots.size(), "WWFF program filter");
        const auto layer = ota::overlay_layer(wwff.spots);
        check(!layer.markers.empty() && layer.markers.front().shape == OverlayShape::Square, "WWFF drawn as squares");
    }

    {
        const QDateTime at = merged.front().time.addSecs(600);
        ota::Filter f;
        check(ota::filter(merged, f, at).size() == merged.size(), "empty filter keeps everything");
        f.program = "SOTA";
        const auto sota_only = ota::filter(merged, f, at);
        bool all_sota = !sota_only.empty();
        for (const auto& s : sota_only) all_sota = all_sota && s.program == "SOTA";
        check(all_sota, "program filter");
        f = {};
        f.mode = "cw";
        bool all_cw = true;
        for (const auto& s : ota::filter(merged, f, at)) all_cw = all_cw && s.mode.toUpper() == "CW";
        check(all_cw, "mode filter is case-insensitive");
        f = {};
        f.band = "20m";
        const auto b20 = ota::filter(merged, f, at);
        bool all20 = true;
        for (const auto& s : b20) all20 = all20 && s.freq_khz >= 14000 && s.freq_khz <= 14350;
        check(all20 && b20.size() < merged.size(), "band filter");
        f = {};
        f.max_age_min = 15;
        const auto young = ota::filter(merged, f, at);
        check(!young.empty() && young.size() < merged.size() && young.front().time == merged.front().time,
              "age filter keeps only recent spots");
        for (const auto& s : young) check(s.time.secsTo(at) <= 15 * 60, "age bound");
    }

    check(!ota::parse_pota("").ok && !ota::parse_pota("{}").ok && !ota::parse_sota("nope").ok, "non-arrays rejected");
    const auto skipped = ota::parse_pota(
        R"([{"activator":"","reference":"US-1","frequency":"7000","spotTime":"2026-10-13T17:00:00"},
            {"activator":"A1B","reference":"US-1","frequency":"x","spotTime":"2026-10-13T17:00:00"},
            {"activator":"A1B","reference":"US-1","frequency":"7000","spotTime":"junk"},
            {"activator":"A1B","reference":"US-1","frequency":"7000","spotTime":"2026-10-13T17:00:00","comments":"QRT"}])");
    check(skipped.ok && skipped.spots.empty(), "malformed rows skipped");

    WidgetContext context;
    bool fail_all = false, fail_pota = false;
    context.transport = [&](const QUrl& url, FetchDone done) {
        FetchResult result;
        if (fail_all || (fail_pota && url.host() == "api.pota.app")) result.error = "Host unreachable";
        else {
            result.http_status = 200;
            const QString name = url.host() == "spots.wwff.co" ? QString("wwff.json") : QFileInfo(url.path()).fileName() + ".json";
            result.body = fixture(dir, name.toUtf8().constData());
        }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    OtaModel model(context);
    auto now = QDateTime(QDate(2026, 10, 13), QTime(17, 45), QTimeZone::utc());
    auto set_clock = [&] {
        model.setClock([&] { return now; });
        model.potaSource().setClock([&] { return now; });
        model.sotaSource().setClock([&] { return now; });
        model.wwffSource().setClock([&] { return now; });
    };
    set_clock();
    QFile::remove(model.potaSource().cachePath());
    QFile::remove(model.sotaSource().cachePath());
    QFile::remove(model.wwffSource().cachePath());
    check(!model.hasData() && model.state() == "loading", "loading before data");
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasData() && model.count() == static_cast<int>(all3.size()), "fresh -> live");
    check(model.statusText() == "POTA + SOTA + WWFF", "all sources named");
    check(model.recent().size() == std::min<int>(30, static_cast<int>(all3.size())), "recent capped at 30");
    {
        size_t positioned = 0;
        for (const auto& sp : all3) positioned += sp.has_position ? 1 : 0;
        check(positioned > 0, "fixture has positioned spots");
        const auto& layers = MapOverlays::instance().layers();
        check(layers.size() == 1 && layers[0].id == "ota" && layers[0].markers.size() == positioned, "live -> POTA spots on the map");
        const auto direct = ota::overlay_layer(merged, 3);  // POTA/SOTA only
        check(direct.markers.size() == 3, "marker cap");
        bool all_pota = true;
        for (const auto& m : direct.markers) all_pota = all_pota && m.shape == OverlayShape::Dot;
        check(all_pota, "SOTA spots (no position) are not drawn");
        model.setMapVisible(false);
        check(MapOverlays::instance().layers().empty(), "map off removes the layer");
        model.setMapVisible(true);
        check(MapOverlays::instance().layers().size() == 1, "map on restores it");
    }

    fail_pota = true;
    now = now.addSecs(20 * 60);
    set_clock();
    model.potaSource().refreshNow();
    model.sotaSource().refreshNow();
    model.wwffSource().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasData(), "POTA down, SOTA still live");

    fail_all = true;
    now = now.addSecs(3600);
    set_clock();
    model.potaSource().refreshNow();
    model.sotaSource().refreshNow();
    model.wwffSource().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.hasData(), "outage -> stale, last data kept");
    check(MapOverlays::instance().layers().empty(), "stale -> markers removed from the map");

    {
        // Settings-driven display filter.
        QTemporaryDir cfg;
        AppSettings settings(cfg.filePath("station.conf"), cfg.path());
        check(!settings.saveOtaFilter("XOTA", "", "", 0).isEmpty() && !settings.saveOtaFilter("", "20 m", "", 0).isEmpty() &&
              !settings.saveOtaFilter("", "", "C W!", 0).isEmpty() && !settings.saveOtaFilter("", "", "", 5000).isEmpty(),
              "bad OTA filter values rejected");
        WidgetContext with_settings;
        with_settings.settings = &settings;
        with_settings.transport = context.transport;
        OtaModel filtered(with_settings);
        check(filtered.filter().program.isEmpty() && filtered.filter().max_age_min == 0, "no filter by default");
        check(settings.saveOtaFilter("pota", "20M", "cw", 90).isEmpty(), "valid OTA filter saved");
        check(filtered.filter().program == "POTA" && filtered.filter().band == "20m" && filtered.filter().mode == "CW" &&
              filtered.filter().max_age_min == 90, "model follows settings");
        AppSettings reloaded(cfg.filePath("station.conf"), cfg.path());
        check(reloaded.otaProgram() == "POTA" && reloaded.otaBand() == "20m" && reloaded.otaMode() == "CW" &&
              reloaded.otaMaxAgeMin() == 90, "OTA filter survives restart");
        check(settings.saveOtaFilter("", "", "", 0).isEmpty() && filtered.filter().program.isEmpty(), "filter cleared");
        check(!settings.saveOtaSort("random").isEmpty(), "bad sort rejected");
        check(settings.saveOtaSort("DIST").isEmpty() && filtered.filter().sort == "dist" && filtered.filter().has_de, "sort followed by model");
        AppSettings again(cfg.filePath("station.conf"), cfg.path());
        check(again.otaSort() == "dist", "sort survives restart");
        check(settings.saveOtaSort("time").isEmpty() && filtered.filter().sort.isEmpty(), "sort cleared");
        {
            ota::Spot near_, far_, none, hi;
            near_.call = "N"; near_.freq_khz = 14000; near_.has_position = true; near_.latitude = 41; near_.longitude = -73;
            far_.call = "F"; far_.freq_khz = 7000; far_.has_position = true; far_.latitude = -33; far_.longitude = 151;
            none.call = "X"; none.freq_khz = 21000;
            hi.call = "H"; hi.freq_khz = 28000; hi.has_position = true; hi.latitude = 40; hi.longitude = -74;
            const std::vector<ota::Spot> in{far_, none, near_, hi};
            const auto now_t = QDateTime::currentDateTimeUtc();
            ota::Filter f;
            f.sort = "dist"; f.has_de = true; f.de_lat = 40.7; f.de_lon = -74.0;
            const auto d = ota::filter(in, f, now_t);
            check(d.size() == 4 && d[0].call == "H" && d[1].call == "N" && d[2].call == "F" && d[3].call == "X", "distance sort, no-position last");
            f.sort = "freq";
            const auto q = ota::filter(in, f, now_t);
            check(q[0].call == "F" && q[3].call == "H", "frequency sort");
            f.sort = "bogus";
            check(ota::filter(in, f, now_t)[0].call == "F", "unknown sort keeps input order");
        }
    }

    std::cout << "OTA: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
