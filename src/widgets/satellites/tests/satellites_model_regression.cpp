// Satellites pane: TLE parsing into next-pass rows, offline.
// Fixture: six sets trimmed from a real CelesTrak amateur-group download captured 2026-10-13.
#include "../satellites_model.hpp"
#include "../../../net/polled_source.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QTimer>
#include <QVariantMap>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    QFile file(QDir(argv[1]).filePath("gp.php"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray body = file.readAll();

    check(satellite_short_name("OSCAR 7 (AO-7)   ") == "AO-7" && satellite_short_name("ISS (ZARYA)") == "ISS" &&
          satellite_short_name("RS-44 & BREEZE-KM R/B") == "RS-44" && satellite_short_name("  FOO  ") == "FOO",
          "short names");

    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());
    WidgetContext context;
    context.settings = &settings;
    bool down = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult r;
        if (down) r.error = "network down"; else { r.http_status = 200; r.body = body; }
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    auto settle = [&] { for (int i = 0; i < 20; ++i) app.processEvents(); };
    const QDateTime t0(QDate(2026, 10, 14), QTime(12, 0), QTimeZone::utc());

    SatellitesModel model(context);
    model.setClock([&] { return t0; });
    model.source().setClock([&] { return t0; });
    QFile::remove(model.source().cachePath());
    // Station not configured yet: data loads but no passes are invented.
    model.start();
    settle();
    check(model.hasData() && model.rows().isEmpty(), "no station -> no rows");

    check(settings.saveStation("W1AW", 45.52, -122.68, 51.5, -0.12, "").isEmpty(), "station saved");
    settle();
    const auto rows = model.rows();
    check(model.state() == "live" && model.hasData(), "live");
    check(rows.size() >= 3 && rows.size() <= 5, "a handful of passes listed within 24 h");
    bool sane = true;
    for (const auto& r : rows) {
        const auto m = r.toMap();
        const int el = m["maxEl"].toInt();
        sane = sane && el >= 10 && el <= 90 && !m["name"].toString().isEmpty() && !m["when"].toString().isEmpty();
    }
    check(sane, "rows have name, time and 10-90 degree peak elevation");

    down = true;
    model.source().refreshNow();
    settle();
    check(model.hasData() && !model.rows().isEmpty(), "outage keeps last elements");

    SatellitesModel bad(context);
    bad.setClock([&] { return t0; });
    bad.source().setClock([&] { return t0; });
    QFile::remove(bad.source().cachePath());
    bad.start();
    settle();
    check(!bad.hasData() && bad.rows().isEmpty() && bad.state() == "offline", "no data and down -> offline, no rows");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
