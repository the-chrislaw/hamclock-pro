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

    check(feed.storms[0].forecast_url.isEmpty(), "trimmed fixture has no forecast advisory entry");
    const auto idx4 = storms::parse(fixture(dir, "CurrentStorms-adv4.json"));
    check(idx4.storms.size() == 1 && idx4.storms[0].forecast_url == "https://www.nhc.noaa.gov/text/MIATCMEP1.shtml" && idx4.storms[0].forecast_adv == "004", "forecast url and advisory number from the index");

    // Real forecast advisory (Trudy, advisory 4, captured 2026-10-14).
    const auto adv = fixture(dir, "MIATCMEP1.shtml");
    const auto issued = QDateTime(QDate(2026, 10, 14), QTime(9, 0), QTimeZone::utc());
    const auto fc = storms::parse_forecast(adv, issued);
    check(fc.ok && fc.advisory == 4 && fc.storm_id == "EP212026" && fc.points.size() == 8, "6 forecast + 2 outlook points");
    check(fc.points[0].valid == QDateTime(QDate(2026, 10, 14), QTime(18, 0), QTimeZone::utc()) && fc.points[0].latitude == 11.5 &&
              fc.points[0].longitude == -100.2 && fc.points[0].wind_kt == 55 && !fc.points[0].outlook, "first point");
    check(fc.points[5].wind_kt == 115 && fc.points[5].latitude == 16.0 && fc.points[5].longitude == -103.6, "sixth point");
    check(!fc.points[5].outlook && fc.points[6].outlook && fc.points[7].outlook && fc.points[7].valid.date() == QDate(2026, 10, 19) && fc.points[7].wind_kt == 85, "outlook points");
    check(!storms::parse_forecast("", issued).ok && !storms::parse_forecast(adv, QDateTime()).ok &&
              !storms::parse_forecast("<html>FORECAST/ADVISORY NUMBER 1</html>", issued).ok, "non-advisory rejected");
    const auto wrap = storms::parse_forecast("FORECAST/ADVISORY NUMBER 9\nEP012026\nFORECAST VALID 01/0600Z 20.0N 150.0E\nMAX WIND 40 KT", QDateTime(QDate(2026, 10, 31), QTime(21, 0), QTimeZone::utc()));
    check(wrap.ok && wrap.points[0].valid.date() == QDate(2026, 11, 1) && wrap.points[0].longitude == 150.0, "day wraps to the next month, E longitude");
    {
        storms::Storm trudy;
        trudy.id = "ep212026";
        trudy.name = "Trudy";
        trudy.wind_kt = 45;
        trudy.latitude = 11.1;
        trudy.longitude = -99.0;
        const std::vector<storms::Storm> one{trudy};
        const auto layer = storms::overlay_layer(one, {{"ep212026", fc}});
        check(layer.markers.size() == 9 && layer.markers[0].prefix == "Trudy", "storm marker first, then 8 forecast dots");
        check(layer.markers[1].prefix.isEmpty() && layer.markers[1].call.isEmpty() && layer.markers[1].shape == OverlayShape::Dot, "forecast dots unlabelled");
        check(layer.markers[5].color == QColor("#d1332b") && layer.markers[1].color == QColor("#e5c32b"), "dot colour follows forecast intensity");
        check(layer.markers[1].detail == "Trudy forecast 14 18:00Z · 55 kt" && layer.markers[8].detail.contains("outlook"), "dot detail");
        check(storms::overlay_layer(one).markers.size() == 1, "no forecast, no dots");
    }

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

    {
        // Same real index for Trudy only, now at advisory 4 (captured 2026-10-14).
        WidgetContext c2;
        c2.transport = [&](const QUrl& url, FetchDone done) {
            FetchResult result;
            result.http_status = 200;
            result.body = url.path().endsWith("CurrentStorms.json") ? fixture(dir, "CurrentStorms-adv4.json") : fixture(dir, QFileInfo(url.path()).fileName().toUtf8().constData());
            QTimer::singleShot(0, [done, result] { done(result); });
        };
        StormsModel m2(c2);
        auto t2 = QDateTime(QDate(2026, 10, 14), QTime(9, 5), QTimeZone::utc());
        m2.setClock([&] { return t2; });
        m2.source().setClock([&] { return t2; });
        QFile::remove(m2.source().cachePath());
        m2.start();
        for (int i = 0; i < 30; ++i) app.processEvents();
        check(m2.state() == "live" && m2.forecasts().size() == 1, "matching advisory -> forecast accepted");
        const auto& layers = MapOverlays::instance().layers();
        check(layers.size() == 1 && layers[0].markers.size() == 9, "storm plus 8 forecast dots on the map");
        t2 = t2.addSecs(7 * 3600);  // forecast page older than its stale limit
        check(m2.forecasts().empty(), "stale forecast is dropped");
    }
    {
        // Index announcing advisory 3 while the text page is already at 4: not drawn.
        WidgetContext c3;
        c3.transport = [&](const QUrl& url, FetchDone done) {
            FetchResult result;
            result.http_status = 200;
            result.body = url.path().endsWith("CurrentStorms.json") ? fixture(dir, "CurrentStorms-adv4.json") : fixture(dir, QFileInfo(url.path()).fileName().toUtf8().constData());
            if (url.path().endsWith("CurrentStorms.json")) result.body.replace("\"advNum\": \"004\"", "\"advNum\": \"003\"");
            QTimer::singleShot(0, [done, result] { done(result); });
        };
        StormsModel m3(c3);
        const auto t3 = QDateTime(QDate(2026, 10, 14), QTime(9, 5), QTimeZone::utc());
        m3.setClock([&] { return t3; });
        m3.source().setClock([&] { return t3; });
        QFile::remove(m3.source().cachePath());
        m3.start();
        for (int i = 0; i < 30; ++i) app.processEvents();
        check(m3.state() == "live" && m3.hasData() && m3.forecasts().empty(), "advisory number mismatch -> no forecast");
    }
    MapOverlays::instance().remove_layer("storms");

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
