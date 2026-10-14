// Earthquakes widget: parser and model, offline.
// Fixture is a trimmed real USGS 2.5_day.geojson response captured 2026-10-13 (feed built 20:53 UTC).
#include "../quakes_model.hpp"
#include "../quakes_overlay.hpp"
#include "../quakes_parse.hpp"
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
    const auto body = fixture(dir, "2.5_day.geojson");
    check(!body.isEmpty(), "fixture present");

    const auto feed = quakes::parse(body);
    check(feed.ok && feed.quakes.size() == 8, "8 quakes parsed");
    check(feed.generated == QDateTime(QDate(2026, 10, 13), QTime(20, 53, 34), QTimeZone::utc()), "feed build time");
    bool newest_first = true;
    for (size_t i = 1; i < feed.quakes.size(); ++i)
        newest_first = newest_first && feed.quakes[i - 1].time >= feed.quakes[i].time;
    check(newest_first, "newest first");
    const auto* top = quakes::strongest(feed);
    check(top && top->magnitude == 6.8 && top->place == "central Mid-Atlantic Ridge", "strongest quake");
    check(top && top->latitude == -0.4085 && top->longitude == -19.4308 && top->depth_km == 10, "lat/lon/depth order");
    check(feed.quakes.front().place == "126 km S of Kokopo, Papua New Guinea", "newest is Kokopo");
    check(quakes::magnitude_class(6.8) == "Strong" && quakes::magnitude_class(7.1) == "Major" &&
          quakes::magnitude_class(5.2) == "Moderate" && quakes::magnitude_class(2.5) == "Minor", "classes");

    check(!quakes::parse("").ok && !quakes::parse("[]").ok && !quakes::parse("{}").ok, "non-feed rejected");
    const auto skipped = quakes::parse(
        R"({"type":"FeatureCollection","features":[
        {"properties":{"mag":null,"time":1},"geometry":{"coordinates":[1,2,3]}},
        {"properties":{"mag":3,"time":"x"},"geometry":{"coordinates":[1,2,3]}},
        {"properties":{"mag":3,"time":1},"geometry":{"coordinates":[1]}},
        {"properties":{"mag":3,"time":1},"geometry":{"coordinates":[200,2,3]}}]})");
    check(skipped.ok && skipped.quakes.empty() && !quakes::strongest(skipped), "malformed rows skipped");

    WidgetContext context;
    bool fail_all = false;
    context.transport = [&](const QUrl& url, FetchDone done) {
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = fixture(dir, QFileInfo(url.path()).fileName().toUtf8().constData()); }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    QuakesModel model(context);
    auto now = QDateTime(QDate(2026, 10, 13), QTime(20, 55), QTimeZone::utc());
    auto set_clock = [&] {
        model.setClock([&] { return now; });
        model.source().setClock([&] { return now; });
    };
    set_clock();
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading", "loading before data");
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasData() && model.count() == 8, "fresh -> live");
    check(model.strongestMagnitude() == 6.8 && model.strongestText().startsWith("Strong · "), "strongest text");
    check(model.recent().size() == 6, "recent capped at 6");
    check(model.updatedAt() == feed.generated, "updatedAt is the feed build time");
    {
        const auto& layers = MapOverlays::instance().layers();
        check(layers.size() == 1 && layers[0].id == "quakes" && layers[0].markers.size() == 8, "live -> quakes on the map");
        const auto& m = layers[0].markers;
        check(m[0].prefix == "M6.8" && m[0].color == QColor("#d1332b"), "strongest first, red");
        bool sorted = true;
        for (size_t i = 1; i < m.size(); ++i) sorted = sorted && m[i - 1].size_px >= m[i].size_px;
        check(sorted, "marker size follows magnitude");
        check(quakes::overlay_layer(feed.quakes, 3).markers.size() == 3, "marker cap");
        model.setMapVisible(false);
        check(MapOverlays::instance().layers().empty(), "map off removes the layer");
        model.setMapVisible(true);
        check(MapOverlays::instance().layers().size() == 1, "map on restores it");
    }

    fail_all = true;
    now = now.addSecs(3 * 3600);
    set_clock();
    model.source().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.hasData(), "outage -> stale, last data kept");
    check(MapOverlays::instance().layers().empty(), "stale -> markers removed from the map");

    std::cout << "Quakes: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
