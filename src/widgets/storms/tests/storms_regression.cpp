// Tropical storms widget: parser and model, offline.
// Fixture is a trimmed real NHC CurrentStorms.json captured 2026-10-13 (two East Pacific storms).
#include "../storms_model.hpp"
#include "../storms_overlay.hpp"
#include "../storms_parse.hpp"
#include "../../../map/overlay.hpp"
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
    const auto body = fixture(dir, "CurrentStorms.json");
    check(!body.isEmpty(), "fixture present");

    const auto feed = storms::parse(body);
    check(feed.ok && feed.storms.size() == 2, "2 storms parsed");
    check(feed.storms[0].name == "Simon" && feed.storms[0].classification == "HU" && feed.storms[0].wind_kt == 80, "strongest first");
    check(feed.storms[0].pressure_mb == 977 && feed.storms[0].latitude == 20.2 && feed.storms[0].longitude == -107.5, "position/pressure");
    check(feed.storms[0].heading_deg == 260 && feed.storms[0].speed_kt == 2, "movement");
    check(feed.storms[0].issued == QDateTime(QDate(2026, 10, 13), QTime(21, 0), QTimeZone::utc()), "issuance time");
    check(storms::category(80) == 1 && storms::category(63) == 0 && storms::category(137) == 5 && storms::category(100) == 3, "categories");
    check(storms::compass(260) == "W" && storms::compass(320) == "NW" && storms::compass(359) == "N" && storms::compass(-1).isEmpty(), "compass");
    check(storms::classification_text("TS") == "Tropical storm", "class text");

    check(!storms::parse("").ok && !storms::parse("[]").ok && !storms::parse("{}").ok, "non-index rejected");
    check(storms::parse(R"({"activeStorms":[]})").ok && storms::parse(R"({"activeStorms":[]})").storms.empty(), "empty list is valid");
    const auto skipped = storms::parse(
        R"({"activeStorms":[{"name":"","intensity":"35","latitudeNumeric":1,"longitudeNumeric":1},
        {"name":"A","intensity":"x","latitudeNumeric":1,"longitudeNumeric":1},
        {"name":"B","intensity":"35","latitudeNumeric":99,"longitudeNumeric":1}]})");
    check(skipped.ok && skipped.storms.empty(), "malformed rows skipped");

    WidgetContext context;
    bool fail_all = false;
    context.transport = [&](const QUrl& url, FetchDone done) {
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = fixture(dir, QFileInfo(url.path()).fileName().toUtf8().constData()); }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    StormsModel model(context);
    auto now = QDateTime(QDate(2026, 10, 13), QTime(21, 5), QTimeZone::utc());
    auto set_clock = [&] {
        model.setClock([&] { return now; });
        model.source().setClock([&] { return now; });
    };
    set_clock();
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading", "loading before data");
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasData() && model.count() == 2, "fresh -> live");
    check(model.headline() == "Simon" && model.headlineIntensity() == "Hurricane cat 1 · 80 kt · 977 mb" && model.headlineCategory() == 1, "headline");
    check(model.headlineDetail() == "moving W 2 kt · 20.2N 107.5W · issued 13 21:00Z", "detail text");
    check(model.others().size() == 1, "one other storm");
    {
        const auto& layers = MapOverlays::instance().layers();
        check(layers.size() == 1 && layers[0].id == "storms" && layers[0].markers.size() == 2, "live -> storms on the map");
        const auto& m = layers[0].markers;
        check(m[0].prefix == "Simon" && m[0].color == QColor("#e08a1e") && m[0].shape == OverlayShape::Diamond, "strongest first, cat 1 orange");
        check(m[0].where.latitude == 20.2 && m[0].where.longitude == -107.5, "marker at storm position");
        check(m[0].size_px > m[1].size_px, "stronger storm drawn larger");
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

    std::cout << "Storms: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
