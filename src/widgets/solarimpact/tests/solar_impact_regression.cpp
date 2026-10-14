// Space weather impact widget: ranking and effect text, offline.
// Fixture is a real NOAA SWPC products/noaa-scales.json captured 2026-10-14 (all quiet).
#include "../solar_impact_model.hpp"
#include "../../../net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
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
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    QFile file(QDir(argv[1]).filePath("noaa-scales.json"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    QByteArray body = file.readAll();

    check(impact_effect('G', 3).contains("50 deg") && impact_effect('R', 0) == "No HF impact" &&
          impact_effect('X', 1).isEmpty() && impact_effect('S', 6).isEmpty() && impact_effect('S', -1).isEmpty(),
          "effect text and bounds");

    WidgetContext context;
    bool down = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult r;
        if (down) r.error = "network down"; else { r.http_status = 200; r.body = body; }
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    auto settle = [&] { for (int i = 0; i < 20; ++i) app.processEvents(); };
    auto now = QDateTime(QDate(2026, 10, 14), QTime(2, 30), QTimeZone::utc());

    SolarImpactModel quiet(context);
    quiet.setClock([&] { return now; });
    quiet.source().setClock([&] { return now; });
    QFile::remove(quiet.source().cachePath());
    check(!quiet.hasData() && quiet.headline().isEmpty() && quiet.ranked().isEmpty(), "nothing before data");
    quiet.start();
    settle();
    check(quiet.state() == "live" && quiet.hasData() && quiet.headline() == "All quiet", "quiet fixture");
    auto rows = quiet.ranked();
    check(rows.size() == 3 && rows[0].toMap()["letter"].toString() == "R" && rows[2].toMap()["letter"].toString() == "G",
          "ties keep R, S, G order");

    // Synthetic storm (not a captured sample): G4 now, R2 now, S0 now but 24 h max S1.
    body = R"({"0":{"DateStamp":"2026-10-14","TimeStamp":"03:00:00","R":{"Scale":"2"},"S":{"Scale":"0"},"G":{"Scale":"4"}},)"
           R"("-1":{"R":{"Scale":"1"},"S":{"Scale":"1"},"G":{"Scale":"3"}}})";
    SolarImpactModel storm(context);
    storm.setClock([&] { return now; });
    storm.source().setClock([&] { return now; });
    QFile::remove(storm.source().cachePath());
    storm.start();
    settle();
    rows = storm.ranked();
    check(rows.size() == 3 && rows[0].toMap()["letter"].toString() == "G" && rows[1].toMap()["letter"].toString() == "R" &&
          rows[2].toMap()["letter"].toString() == "S", "ranked G, R, S by current level");
    check(rows[0].toMap()["word"].toString() == "Severe" && rows[2].toMap()["max24"].toInt() == 1, "word and 24 h max");
    check(storm.headline().startsWith("G4 Severe: Aurora to about 45 deg"), "headline names the worst");

    // Equal current level: the higher 24 h max ranks first.
    body = R"({"0":{"DateStamp":"2026-10-14","TimeStamp":"03:00:00","R":{"Scale":"0"},"S":{"Scale":"0"},"G":{"Scale":"0"}},)"
           R"("-1":{"R":{"Scale":"0"},"S":{"Scale":"2"},"G":{"Scale":"0"}}})";
    SolarImpactModel tie(context);
    tie.setClock([&] { return now; });
    tie.source().setClock([&] { return now; });
    QFile::remove(tie.source().cachePath());
    tie.start();
    settle();
    check(tie.ranked()[0].toMap()["letter"].toString() == "S" && tie.headline() == "All quiet", "24 h max breaks ties");

    now = now.addSecs(4 * 3600);
    down = true;
    storm.source().refreshNow();
    settle();
    check(storm.state() == "stale" && storm.hasData(), "outage -> stale, last values kept");

    SolarImpactModel offline(context);
    offline.setClock([&] { return now; });
    offline.source().setClock([&] { return now; });
    QFile::remove(offline.source().cachePath());
    offline.start();
    settle();
    check(offline.state() == "offline" && !offline.hasData() && offline.headline().isEmpty(), "no data and down -> offline");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
