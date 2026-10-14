// Active fires widget: parser, model and overlay, offline.
// Fixture: 40 rows trimmed from the real NASA FIRMS J1_VIIRS_C2_USA_contiguous_and_Hawaii_24h.csv
// captured 2026-10-14 (1300 rows in the full file).
#include "../fires_model.hpp"
#include "../fires_overlay.hpp"
#include "../fires_parse.hpp"
#include "../../../map/overlay.hpp"
#include "../../../net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimeZone>
#include <QTimer>
#include <algorithm>
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
    const auto body = fixture(dir, "J1_VIIRS_C2_USA_contiguous_and_Hawaii_24h.csv");
    check(!body.isEmpty(), "fixture present");

    const auto feed = fires::parse(body);
    check(feed.ok && feed.detections.size() == 40, "40 detections parsed");
    check(feed.detections[0].frp_mw == 120.09 && feed.detections[0].latitude == 32.11736 && feed.detections[0].longitude == -86.79617, "strongest first");
    check(feed.detections[0].acquired == QDateTime(QDate(2026, 10, 13), QTime(18, 38), QTimeZone::utc()), "acquisition time UTC");
    check(feed.detections[0].confidence == fires::Detection::Low && feed.detections[1].confidence == fires::Detection::Nominal, "confidence levels");

    check(!fires::parse("").ok && !fires::parse("<html>").ok && !fires::parse("a,b\n1,2").ok, "non-FIRMS body rejected");
    const QByteArray head = "latitude,longitude,acq_date,acq_time,confidence,frp\n";
    check(fires::parse(head).ok && fires::parse(head).detections.empty(), "header only is valid and empty");
    const auto skipped = fires::parse(head + "x,1,2026-10-13,0054,h,1\n95,1,2026-10-13,0054,h,1\n1,1,bad,0054,h,1\n1,1,2026-10-13,0054,h\n1,1,2026-10-13,54,h,-1\n");
    check(skipped.ok && skipped.detections.empty(), "malformed rows skipped");
    const auto padded = fires::parse(head + "1,2,2026-10-13,54,high,3\n");
    check(padded.detections.size() == 1 && padded.detections[0].acquired.time() == QTime(0, 54) && padded.detections[0].confidence == fires::Detection::High, "short time padded");

    WidgetContext context;
    bool fail_all = false;
    context.transport = [&](const QUrl& url, FetchDone done) {
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = fixture(dir, QFileInfo(url.path()).fileName().toUtf8().constData()); }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    FiresModel model(context);
    auto now = QDateTime(QDate(2026, 10, 14), QTime(4, 0), QTimeZone::utc());
    auto set_clock = [&] {
        model.setClock([&] { return now; });
        model.source().setClock([&] { return now; });
    };
    set_clock();
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading", "loading before data");
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasData() && model.count() == 40, "fresh -> live");
    check(model.summary() == "40 detections", "summary");
    check(model.highCount() > 0 && model.highCount() < 40, "high-confidence count");
    check(model.detail().startsWith("strongest 120 MW at 32.1N 86.8W · newest 13 "), "detail text");
    {
        const auto& layers = MapOverlays::instance().layers();
        check(layers.size() == 1 && layers[0].id == "fires", "live -> fires on the map");
        check(!layers[0].markers.empty() && layers[0].markers.size() <= fires::kMaxMarkers, "markers capped");
        size_t not_low = 0;
        for (const auto& d : feed.detections) not_low += d.confidence != fires::Detection::Low;
        check(layers[0].markers.size() == std::min(not_low, fires::kMaxMarkers) && not_low < 40, "low confidence left out");
        check(layers[0].markers[0].color == QColor("#d1332b"), "strong fire red");
        model.setMapVisible(false);
        check(MapOverlays::instance().layers().empty(), "map off removes the layer");
        model.setMapVisible(true);
        check(MapOverlays::instance().layers().size() == 1, "map on restores it");
    }

    fail_all = true;
    now = now.addSecs(13 * 3600);
    set_clock();
    model.source().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.hasData(), "outage -> stale, last data kept");
    check(MapOverlays::instance().layers().empty(), "stale -> markers removed from the map");

    std::cout << "Fires: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
