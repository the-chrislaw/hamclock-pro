// PSKReporter widget: parser, query URL and model, offline.
// Fixture is a trimmed real response captured 2026-10-13 ~17:40 UTC.
#include "../psk_model.hpp"
#include "../psk_parse.hpp"
#include "../psk_overlay.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QFile>
#include <QTimeZone>
#include <QTimer>
#include <cmath>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    QFile file(QDir(argv[1]).filePath("query.xml"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray body = file.readAll();

    const auto feed = psk::parse_reports(body);
    check(feed.ok && feed.reports.size() == 1, "one report");
    const auto& r = feed.reports.front();
    check(r.receiver == "MM0ZBH" && r.receiver_grid == "IO86IB" && r.sender == "W1AW", "calls and grids");
    check(r.freq_khz == 21029.304 && r.band == "15m" && r.mode == "CW", "frequency, band, mode");
    check(r.has_snr && r.snr_db == 14, "SNR");
    check(r.time == QDateTime::fromSecsSinceEpoch(1791913278, QTimeZone::utc()), "time is UTC");

    check(psk::parse_reports("<receptionReports/>").ok && psk::parse_reports("<receptionReports/>").reports.empty(),
          "empty document is ok");
    check(!psk::parse_reports("").ok && !psk::parse_reports("<html><p>").ok && !psk::parse_reports("nope").ok,
          "non-reports rejected");
    const auto mixed = psk::parse_reports(
        R"(<receptionReports>
          <receptionReport receiverCallsign="" frequency="14074000" flowStartSeconds="100" />
          <receptionReport receiverCallsign="A1B" frequency="x" flowStartSeconds="100" />
          <receptionReport receiverCallsign="A1B" frequency="14074000" flowStartSeconds="0" />
          <receptionReport receiverCallsign="old1" frequency="14074000" flowStartSeconds="1000" sNR="-12" />
          <receptionReport receiverCallsign="new1" frequency="7074000" flowStartSeconds="2000" />
        </receptionReports>)");
    check(mixed.ok && mixed.reports.size() == 2, "malformed rows skipped");
    check(mixed.reports[0].receiver == "NEW1" && !mixed.reports[0].has_snr, "newest first, SNR optional");

    {
        std::vector<psk::Report> rs(4);
        rs[0].receiver = "A1"; rs[0].receiver_grid = "IO86";
        rs[1].receiver = "A1"; rs[1].receiver_grid = "FN31";   // duplicate call: first wins
        rs[2].receiver = "B2"; rs[2].receiver_grid = "IO86IB";
        rs[3].receiver = "C3"; rs[3].receiver_grid = "IO";     // too coarse
        const auto layer = psk::overlay_layer(rs);
        check(layer.markers.size() == 2 && layer.id == "psk", "overlay: dedup and coarse grids skipped");
        check(std::abs(layer.markers[0].where.latitude - 56.5) < 1e-9 && std::abs(layer.markers[0].where.longitude + 3.0) < 1e-9 &&
                  layer.markers[0].path_from_de,
              "overlay: 4-char grid centre, path from DE");
        check(std::abs(layer.markers[1].where.latitude - (56.0 + 1 * 2.5 / 60 + 1.25 / 60)) < 1e-6, "overlay: 6-char centre");
        check(psk::overlay_layer(rs, 1).markers.size() == 1, "overlay: capped");
    }
    {
        std::vector<psk::Report> rs(5);
        rs[0].band = "20m"; rs[1].band = "20m"; rs[2].band = "40m"; rs[3].band = "20m"; rs[4].band = "";
        rs[0].freq_khz = 14074; rs[1].freq_khz = 14080; rs[2].freq_khz = 7040; rs[3].freq_khz = 14090; rs[4].freq_khz = 5000;
        const auto stats = psk::band_stats(rs);
        check(stats.size() == 2 && stats[0].first == "20m" && stats[0].second == 3 && stats[1].first == "40m" &&
                  stats[1].second == 1,
              "band stats: most reports first, out-of-band skipped");
        check(psk::band_stats({}).empty(), "band stats: empty");
        check(psk::band_summary(stats, 1) == "20m 3" && psk::band_summary(stats, 5) == "20m 3 · 40m 1", "band summary");
    }
    {
        std::vector<psk::Report> rs(4);
        rs[0].mode = "ft8"; rs[1].mode = "CW"; rs[2].mode = "FT8"; rs[3].mode = "";
        const auto ms = psk::mode_stats(rs);
        check(ms.size() == 2 && ms[0].first == "FT8" && ms[0].second == 2 && ms[1].first == "CW",
              "mode stats: case folded, busiest first, empty skipped");
        check(psk::band_summary(ms, 4) == "FT8 2 · CW 1" && psk::mode_stats({}).empty(), "mode summary");
    }
    {
        std::vector<psk::Report> rs(4);
        rs[0].receiver = "NEAR"; rs[0].receiver_grid = "IO91";
        rs[1].receiver = "FAR"; rs[1].receiver_grid = "QM05ab"; rs[1].band = "20m";
        rs[2].receiver = "BAD"; rs[2].receiver_grid = "ZZ";
        rs[3].receiver = "COARSE"; rs[3].receiver_grid = "QM";
        const auto f = psk::farthest_report(rs, 51.5, -0.1);
        check(f.found && f.call == "FAR" && f.band == "20m" && f.km > 9000 && f.km < 10500, "farthest: Japan from London");
        check(!psk::farthest_report({}, 0, 0).found && !psk::farthest_report({rs[2], rs[3]}, 0, 0).found, "farthest: none");
        const auto near = psk::within_distance(rs, 51.5, -0.1, 2000);
        check(near.size() == 1 && near[0].receiver == "NEAR", "within 2000 km: only the near grid; bad/coarse grids dropped");
        check(psk::within_distance(rs, 51.5, -0.1, 12000).size() == 2, "within 12000 km: near and far");
        check(psk::within_distance(rs, 51.5, -0.1, 0).size() == 4, "limit 0 keeps everything");
    }
    {
        std::vector<psk::Report> rs(3);
        rs[0].receiver = "VK2ABC"; rs[0].receiver_grid = "QF56";
        rs[1].receiver = "JA1XYZ"; rs[1].receiver_grid = "PM95ab";
        rs[2].receiver = "G4ZZZ"; rs[2].receiver_grid = "io91";
        const auto t = psk::match_tokens(" vk,JA; pm95 bad! vk ");
        check(t == QStringList({"VK", "JA", "PM95"}), "match tokens: normalised, deduped, junk dropped");
        check(psk::matching(rs, t).size() == 2 && psk::matching(rs, {}).size() == 3, "matching by call prefix");
        check(psk::matching(rs, psk::match_tokens("IO91")).size() == 1, "matching by grid prefix, case folded");
        check(psk::matching(rs, psk::match_tokens("ZL")).empty(), "matching: none");
    }
    check(psk::query_url("w1aw").contains("senderCallsign=W1AW&") && psk::query_url("w1aw").contains("appcontact="),
          "query URL");
    check(psk::query_url("EA8/W1AW").contains("EA8%2FW1AW"), "slash encoded");
    check(psk::query_url("").isEmpty() && psk::query_url("A B&x=1").isEmpty() && psk::query_url("W1AW-5").isEmpty(),
          "unsafe calls refused");

    QString requested;
    WidgetContext context;
    context.transport = [&](const QUrl& url, FetchDone done) {
        requested = url.toString();
        FetchResult result;
        result.http_status = 200;
        result.body = body;
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    QTemporaryDir tmp;
    AppSettings settings(tmp.filePath("station.json"), tmp.path());
    context.settings = &settings;
    PskModel model(context);
    auto now = QDateTime::fromSecsSinceEpoch(1791913300, QTimeZone::utc());
    model.setClock([&] { return now; });
    model.start();
    check(model.state() == "unconfigured" && requested.isEmpty(), "no callsign -> unconfigured, no request");

    check(settings.saveStation("W1AW", 41.7, -72.7, 51.5, -0.1, "").isEmpty(), "save callsign");
    check(!settings.savePskMaxKm(-1).isEmpty() && !settings.savePskMaxKm(20001).isEmpty() && settings.pskMaxKm() == 0,
          "invalid PSK distance rejected");
    check(settings.savePskMaxKm(5000).isEmpty() && settings.pskMaxKm() == 5000, "PSK distance saved");
    check(settings.savePskMatch(" vk, ja ").isEmpty() && settings.pskMatch() == "VK, JA", "PSK filter saved normalised");
    check(settings.savePskMatch("").isEmpty() && settings.pskMatch().isEmpty(), "PSK filter reset");
    check(settings.savePskMaxKm(0).isEmpty() && settings.pskMaxKm() == 0, "PSK distance reset");
    for (int i = 0; i < 20; ++i) app.processEvents();
    if (model.source()) QFile::remove(model.source()->cachePath());
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(requested.contains("senderCallsign=W1AW"), "request uses the settings callsign");
    check(model.state() == "live" && model.hasData() && model.count() == 1, "report -> live");
    check(model.modes() == "CW 1", "model modes line");
    check(model.bestSnr() == "14 dB" && model.recent().size() == 1, "summary");

    check(MapOverlays::instance().layers().size() == 1 && MapOverlays::instance().layers()[0].id == "psk",
          "live feed publishes the overlay layer");
    model.setMapVisible(false);
    check(MapOverlays::instance().layers().empty(), "map off removes the layer");
    model.setMapVisible(true);
    check(settings.mapSourceVisible("psk") && !settings.saveMapSourceVisible("nope", false).isEmpty(),
          "map source defaults on, unknown key rejected");
    check(settings.saveMapSourceVisible("psk", false).isEmpty() && !settings.mapSourceVisible("psk") &&
              MapOverlays::instance().layers().empty(),
          "Settings toggle hides the PSK layer");
    check(settings.saveMapSourceVisible("psk", true).isEmpty() && MapOverlays::instance().layers().size() == 1,
          "Settings toggle shows the PSK layer again");
    check(settings.mapSourceLabel("psk") == -1 && !settings.saveMapSourceLabel("psk", 9).isEmpty() &&
              !settings.saveMapSourceLabel("nope", 1).isEmpty(),
          "label style defaults to the layer's own; bad input rejected");
    check(settings.saveMapSourceLabel("psk", int(OverlayLabelMode::Dot)).isEmpty() && settings.saveMapMinLabelPx(70).isEmpty() &&
              MapOverlays::instance().layers()[0].labels == OverlayLabelMode::Dot &&
              MapOverlays::instance().layers()[0].min_label_distance_px == 70,
          "Settings label style and distance reach the PSK layer");
    check(!settings.saveMapMinLabelPx(500).isEmpty(), "label distance range checked");
    settings.saveMapSourceLabel("psk", -1);
    settings.saveMapMinLabelPx(0);
    check(MapOverlays::instance().layers()[0].labels == OverlayLabelMode::Call &&
              MapOverlays::instance().layers()[0].min_label_distance_px == 44,
          "reset restores the layer's own style");
    now = now.addSecs(25 * 60);
    model.source()->setClock([&] { return now; });
    emit model.ageChanged();
    check(model.state() == "stale", "old data -> stale");
    check(MapOverlays::instance().layers().empty(), "stale feed removes the layer");

    check(settings.saveStation("W1AW-5", 41.7, -72.7, 51.5, -0.1, "").isEmpty(), "save unsupported callsign");
    model.start();
    check(model.state() == "unconfigured" && model.count() == 0, "unsupported callsign -> unconfigured, list dropped");
    if (failures) std::cerr << failures << " of " << checks << " checks failed\n";
    return failures ? 1 : 0;
}
