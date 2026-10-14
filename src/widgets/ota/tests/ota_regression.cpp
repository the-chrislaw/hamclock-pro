// On-the-air widget: POTA/SOTA parsers and model, offline.
// Fixtures are trimmed real responses captured 2026-10-13 ~17:40 UTC.
#include "../ota_model.hpp"
#include "../ota_overlay.hpp"
#include "../ota_parse.hpp"
#include "../../../net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
        else { result.http_status = 200; result.body = fixture(dir, (QFileInfo(url.path()).fileName() + ".json").toUtf8().constData()); }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    OtaModel model(context);
    auto now = QDateTime(QDate(2026, 10, 13), QTime(17, 45), QTimeZone::utc());
    auto set_clock = [&] {
        model.setClock([&] { return now; });
        model.potaSource().setClock([&] { return now; });
        model.sotaSource().setClock([&] { return now; });
    };
    set_clock();
    QFile::remove(model.potaSource().cachePath());
    QFile::remove(model.sotaSource().cachePath());
    check(!model.hasData() && model.state() == "loading", "loading before data");
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasData() && model.count() == static_cast<int>(merged.size()), "fresh -> live");
    check(model.statusText() == "POTA + SOTA", "both sources named");
    check(model.recent().size() == 6, "recent capped at 6");
    {
        size_t positioned = 0;
        for (const auto& sp : merged) positioned += sp.has_position ? 1 : 0;
        check(positioned > 0, "fixture has positioned spots");
        const auto& layers = MapOverlays::instance().layers();
        check(layers.size() == 1 && layers[0].id == "ota" && layers[0].markers.size() == positioned, "live -> POTA spots on the map");
        const auto direct = ota::overlay_layer(merged, 3);
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
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasData(), "POTA down, SOTA still live");

    fail_all = true;
    now = now.addSecs(3600);
    set_clock();
    model.potaSource().refreshNow();
    model.sotaSource().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.hasData(), "outage -> stale, last data kept");
    check(MapOverlays::instance().layers().empty(), "stale -> markers removed from the map");

    std::cout << "OTA: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
