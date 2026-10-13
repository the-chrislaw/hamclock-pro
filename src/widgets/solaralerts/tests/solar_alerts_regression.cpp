// Space alerts widget: parser and model, offline.
// Fixture is a trimmed real NOAA SWPC products/alerts.json captured 2026-10-13.
#include "../solar_alerts_model.hpp"
#include "../solar_alerts_parse.hpp"
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
QDateTime utc(int d, int h, int m = 0) { return QDateTime(QDate(2026, 10, d), QTime(h, m), QTimeZone::utc()); }
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    QFile file(QDir(argv[1]).filePath("alerts.json"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray body = file.readAll();

    const auto all = solaralerts::parse_alerts(body);
    check(all.size() == 12, "all 12 messages parsed");
    bool sorted = true;
    for (size_t i = 1; i < all.size(); ++i) sorted = sorted && all[i - 1].issued >= all[i].issued;
    check(sorted, "newest first");
    check(all[0].code == "K04W" && all[0].kind == "Extended warning" && all[0].headline == "Geomagnetic K-index of 4 expected"
          && all[0].until == utc(13, 21), "extended warning: headline and new end time");
    check(all[1].kind == "Warning" && all[1].until == utc(13, 15), "warning: Valid To");
    check(all[2].kind == "Watch" && all[2].scale == "G1" && !all[2].until.isValid(), "watch: scale from category");
    check(all[3].cancelled && all[3].kind == "Cancel", "cancel notice recognised");
    check(all[5].kind == "Alert" && all[5].scale == "G1" && all[5].issued == utc(10, 21, 45), "alert: scale and issue time");
    check(all[6].scale == "G3" && solaralerts::alert_level(all[6]) == 2, "G3 is strong");
    check(solaralerts::alert_level(all[5]) == 1 && solaralerts::alert_level(all[0]) == 1, "G1 and warnings are level 1");

    // Oct 13 15:00: the K04W warning (extended to 21:00) and the G1 watch are in force;
    // the cancelled G2 watch, three-day-old alerts and the expired K07W are not.
    auto active = solaralerts::active_alerts(all, utc(13, 15));
    check(active.size() == 2 && active[0].code == "K04W" && active[1].code == "A20F", "active set at 15:00");
    check(solaralerts::active_alerts(all, utc(13, 22)).empty(), "warning drops after its end time, watch after 48 h");
    check(solaralerts::active_alerts(all, utc(10, 22)).size() == 4, "during the Oct 10 storm: one row per code, later messages ignored");
    check(solaralerts::active_alerts(all, utc(16, 0)).empty(), "watch expires after 48 h");

    check(solaralerts::parse_alerts("").empty() && solaralerts::parse_alerts("<html>").empty() &&
          solaralerts::parse_alerts("[{\"product_id\":\"X\",\"message\":\"junk\"},5]").empty(), "garbage rejected");

    // Model.
    WidgetContext context;
    bool down = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult r;
        if (down) r.error = "network down"; else { r.http_status = 200; r.body = body; }
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    auto settle = [&] { for (int i = 0; i < 20; ++i) app.processEvents(); };
    SolarAlertsModel model(context);
    auto now = utc(13, 15);
    model.setClock([&] { return now; });
    model.source().setClock([&] { return now; });
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading", "loading before data");
    model.start();
    settle();
    check(model.state() == "live" && model.hasData() && model.count() == 2, "live with two alerts");
    const auto rows = model.alerts();
    const auto first = rows[0].toMap();
    check(first["headline"].toString() == "Geomagnetic K-index of 4 expected" && first["until"].toString() == "until 21:00Z"
          && first["issued"].toString() == "14:31Z", "row text");
    check(rows[1].toMap()["scale"].toString() == "G1", "watch row scale");

    now = utc(13, 22);
    emit model.ageChanged();
    check(model.count() == 0 && model.hasData(), "expiry follows the clock without a refetch");

    now = utc(13, 15).addSecs(4 * 3600);
    down = true;
    model.source().refreshNow();
    settle();
    check(model.state() == "stale" && model.hasData(), "outage -> stale, last list kept");

    SolarAlertsModel empty(context);
    empty.setClock([&] { return now; });
    empty.source().setClock([&] { return now; });
    QFile::remove(empty.source().cachePath());
    empty.start();
    settle();
    check(empty.state() == "offline" && !empty.hasData() && empty.count() == 0, "no data and down -> offline, not 'nothing in force'");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
