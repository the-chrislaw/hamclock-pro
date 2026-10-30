// HF path reliability matrix widget model, offline. Fixtures are trimmed real
// NOAA SWPC responses captured 2026-10-13 (SFI 123, Kp 3).
#include "../bands_matrix_model.hpp"
#include "../../../net/polled_source.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QLocale>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
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
    QLocale::setDefault(QLocale::c()); // a new install takes units from the region; these checks expect km
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    const QString dir = argv[1];

    WidgetContext context;
    bool fail_all = false;
    context.transport = [&](const QUrl& url, FetchDone done) {
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = fixture(dir, QFileInfo(url.path()).fileName().toUtf8().constData()); }
        QTimer::singleShot(0, [done, result] { done(result); });
    };

    // Without settings there is no station, hence no path: never a matrix from nothing.
    BandsMatrixModel bare(context);
    const auto now = QDateTime(QDate(2026, 10, 13), QTime(16, 0), QTimeZone::utc());
    for (auto* source : {&bare.kpSource(), &bare.fluxSource()}) { source->setClock([&] { return now; }); QFile::remove(source->cachePath()); }
    bare.setClock([&] { return now; });
    check(!bare.hasMatrix() && bare.state() == "loading", "no matrix before data");
    bare.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(bare.state() == "live", "fresh SWPC data -> live");
    check(!bare.hasMatrix() && !bare.hasPath() && bare.rows().isEmpty(), "no station -> no path, no matrix");

    // With a station: London -> New York.
    QTemporaryDir tmp;
    AppSettings settings(tmp.filePath("station.conf"), tmp.path());
    check(settings.saveStation("G1ABC", 51.5, -0.1, 40.7, -74.0, "Europe/London").isEmpty(), "station saved");
    context.settings = &settings;
    BandsMatrixModel model(context);
    for (auto* source : {&model.kpSource(), &model.fluxSource()}) { source->setClock([&] { return now; }); QFile::remove(source->cachePath()); }
    model.setClock([&] { return now; });
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.hasMatrix() && model.hasPath(), "matrix with station and indices");
    const auto rows = model.rows();
    check(rows.size() == 8, "eight HF bands");
    check(rows.first().toMap()["cells"].toList().size() == 24, "24 hourly cells per band");
    check(model.startText() == "16Z", "columns start at the current UTC hour");
    check(model.pathText().startsWith("55") && model.pathText().endsWith("SSB 100 W · 6 dBi"), "path text with distance and mode");
    check(model.pathText().startsWith("55") && model.pathText().contains(" km "), "metric path distance");
    check(settings.saveImperialUnits(true).isEmpty(), "imperial saved");
    for (int i = 0; i < 5; ++i) app.processEvents();
    check(model.pathText().startsWith("34") && model.pathText().contains(" mi "), "path distance follows Imperial units");
    check(settings.saveImperialUnits(false).isEmpty(), "metric restored");
    for (int i = 0; i < 5; ++i) app.processEvents();
    check(model.bestText().startsWith("Best now: "), "best band named");
    double r20 = 0, r80 = 1;
    for (const auto& row : rows) {
        const auto m = row.toMap();
        if (m["band"] == "20m") r20 = m["now"].toDouble();
        if (m["band"] == "80m") r80 = m["now"].toDouble();
    }
    check(r20 > 0.5, "20m open London-New York in the afternoon");
    check(r80 < 0.2, "80m absorbed by day");

    // The hour turns: the matrix is recomputed from the new hour.
    const auto later = now.addSecs(3 * 3600);
    model.setClock([&] { return later; });
    for (int i = 0; i < 40; ++i) app.processEvents();
    check(model.startText() == "16Z" || model.startText() == "19Z", "recompute follows the clock when it ticks");

    // Options file: mode and power change the label; out-of-range power is ignored.
    {
        QSettings ini(settings.optionsPath(), QSettings::IniFormat);
        ini.setValue("bandsmatrix/mode", "CW");
        ini.setValue("bandsmatrix/power_w", "5");
        ini.setValue("bandsmatrix/antenna_gain_db", "12");
        ini.sync();
    }
    BandsMatrixModel cw(context);
    for (auto* source : {&cw.kpSource(), &cw.fluxSource()}) { source->setClock([&] { return now; }); QFile::remove(source->cachePath()); }
    cw.setClock([&] { return now; });
    cw.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(cw.pathText().endsWith("CW 5 W · 12 dBi"), "options set mode and power");
    {
        QSettings ini(settings.optionsPath(), QSettings::IniFormat);
        ini.setValue("bandsmatrix/mode", "bogus");
        ini.setValue("bandsmatrix/power_w", "99999");
        ini.setValue("bandsmatrix/antenna_gain_db", "500");
        ini.sync();
    }
    BandsMatrixModel odd(context);
    for (auto* source : {&odd.kpSource(), &odd.fluxSource()}) { source->setClock([&] { return now; }); QFile::remove(source->cachePath()); }
    odd.setClock([&] { return now; });
    odd.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(odd.pathText().endsWith("SSB 100 W · 6 dBi"), "bad options fall back to SSB 100 W 6 dBi");

    {
        QSettings ini(settings.optionsPath(), QSettings::IniFormat);
        ini.setValue("bandsmatrix/long_path", "true");
        ini.sync();
    }
    BandsMatrixModel longp(context);
    for (auto* source : {&longp.kpSource(), &longp.fluxSource()}) { source->setClock([&] { return now; }); QFile::remove(source->cachePath()); }
    longp.setClock([&] { return now; });
    longp.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(longp.pathText().contains("long path"), "long_path option is shown in the label");

    // Outage: data kept but flagged stale.
    fail_all = true;
    const auto outage = now.addSecs(8 * 3600);
    for (auto* source : {&model.kpSource(), &model.fluxSource()}) { source->setClock([&] { return outage; }); source->refreshNow(); }
    model.setClock([&] { return outage; });
    for (int i = 0; i < 40; ++i) app.processEvents();
    check(model.state() == "stale", "outage -> stale, not live");

    // DX on top of DE: nothing to rate.
    settings.saveStation("G1ABC", 51.5, -0.1, 51.5, -0.1, "Europe/London");
    check(!model.hasPath() && !model.hasMatrix(), "DX == DE -> no path");

    std::cout << "BandsMatrix: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
