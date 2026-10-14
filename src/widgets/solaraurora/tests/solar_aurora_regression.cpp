// Aurora widget: parser, map style and model, offline.
// Fixture is a real NOAA SWPC json/ovation_aurora_latest.json captured 2026-10-14, trimmed to
// |lat| >= 55, even longitudes and non-zero cells.
#include "../solar_aurora_model.hpp"
#include "../solar_aurora_parse.hpp"
#include "../../../map/styles.hpp"
#include "../../../net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTimeZone>
#include <QTimer>
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
    QFile file(QDir(argv[1]).filePath("ovation_aurora_latest.json"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray body = file.readAll();

    const auto g = solaraurora::parse_ovation(body);
    check(g.valid && g.observed == QDateTime(QDate(2026, 10, 14), QTime(4, 57), QTimeZone::utc()), "observation time");
    check(g.forecast == QDateTime(QDate(2026, 10, 14), QTime(5, 46), QTimeZone::utc()), "forecast time");
    check(g.at(-90 + 35, 0) >= 0 && solaraurora::peak(g, true) > 0 && solaraurora::peak(g, true) <= 100, "northern peak in range");
    check(solaraurora::peak(g, false) > 0, "southern peak present");

    const auto small = solaraurora::parse_ovation(
        R"({"Observation Time":"2026-10-14T04:57:00Z","Forecast Time":"2026-10-14T05:46:00Z",)"
        R"("coordinates":[[0,65,40],[180,-60,12],[10,95,50],[5,70,"x"],[400,10,9]]})");
    check(small.valid && small.at(65, 0) == 40 && small.at(-60, 180) == 12 && small.at(70, 5) == 0, "cells placed, bad rows skipped");
    check(solaraurora::peak(small, true) == 40 && solaraurora::peak(small, false) == 12, "peaks per hemisphere");
    const auto img = solaraurora::to_image(small);
    check(img.size() == QSize(360, 180), "image size");
    check(img.pixelColor(180, 25).alpha() > 0 && img.pixelColor(0, 25).alpha() == 0, "lon 0 at centre column, lat 65 at row 25");
    check(img.pixelColor(0, 150).alpha() > 0, "lon 180 on the left edge, lat -60 at row 150");
    check(!solaraurora::parse_ovation("").valid && !solaraurora::parse_ovation("<html>").valid &&
          !solaraurora::parse_ovation("{}").valid && !solaraurora::parse_ovation("[]").valid, "garbage rejected");

    WidgetContext context;
    bool down = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult r;
        if (down) r.error = "network down"; else { r.http_status = 200; r.body = body; }
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    auto settle = [&] { for (int i = 0; i < 20; ++i) app.processEvents(); };
    auto now = QDateTime(QDate(2026, 10, 14), QTime(5, 10), QTimeZone::utc());
    auto has_style = [] {
        for (const auto& s : MapStyles::instance().styles()) if (s.id == "aurora") return !s.image.isNull();
        return false;
    };
    SolarAuroraModel model(context);
    model.setClock([&] { return now; });
    model.source().setClock([&] { return now; });
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading" && !has_style(), "loading before data");
    model.start();
    settle();
    check(model.state() == "live" && model.hasData() && model.observed() == "04:57Z", "live");
    check(model.northPeak() > 0 && has_style(), "aurora map style published");

    now = now.addSecs(2 * 3600);
    down = true;
    model.source().refreshNow();
    settle();
    check(model.state() == "stale" && model.hasData() && has_style(), "outage within 3 h -> stale, values kept");

    now = now.addSecs(2 * 3600);  // observation now 4 h 13 min old
    down = false;
    model.source().refreshNow();
    settle();
    check(!model.hasData() && !has_style(), "old nowcast never shown or drawn as current");

    SolarAuroraModel offline(context);
    down = true;
    offline.setClock([&] { return now; });
    offline.source().setClock([&] { return now; });
    QFile::remove(offline.source().cachePath());
    offline.start();
    settle();
    check(offline.state() == "offline" && !offline.hasData(), "no data and down -> offline");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
