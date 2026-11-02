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

namespace {
QByteArray kp_row(const QDateTime& time, double kp = 3.0) {
    return QString(R"([{"time_tag":"%1","Kp":%2,"a_running":15,"station_count":8}])")
        .arg(time.toString("yyyy-MM-ddTHH:mm:ss")).arg(kp).toUtf8();
}
QByteArray flux_row(const QDateTime& time) {
    return QString(R"([{"flux":123,"time_tag":"%1"}])").arg(time.toString("yyyy-MM-ddTHH:mm:ss")).toUtf8();
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
    QByteArray flux_override, kp_override;  // when set, the feed answers with this instead of the fixture
    context.transport = [&](const QUrl& url, FetchDone done) {
        FetchResult result;
        const auto name = QFileInfo(url.path()).fileName();
        if (fail_all) result.error = "Host unreachable";
        else {
            result.http_status = 200;
            const auto& override_body = name == "10cm-flux.json" ? flux_override : kp_override;
            result.body = !override_body.isEmpty() ? override_body : fixture(dir, name.toUtf8().constData());
        }
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
    emit model.ageChanged();
    check(model.startText() == "19Z", "the next tick after the hour turns moves the columns");

    // New SFI in the same UTC hour reaches the path pane at once, not at the next hour.
    {
        BandsMatrixModel fresh(context);
        for (auto* source : {&fresh.kpSource(), &fresh.fluxSource()}) { source->setClock([&] { return now; }); QFile::remove(source->cachePath()); }
        fresh.setClock([&] { return now; });
        fresh.start();
        for (int i = 0; i < 20; ++i) app.processEvents();
        check(fresh.hasMatrix(), "matrix before the new SFI");
        const auto before = fresh.rows();
        int changes = 0;
        QObject::connect(&fresh, &BandsMatrixModel::matrixChanged, &fresh, [&] { ++changes; });
        flux_override = R"([{"flux":230,"time_tag":"2026-10-12T21:00:00"}])";
        fresh.fluxSource().refreshNow();
        for (int i = 0; i < 20; ++i) app.processEvents();
        flux_override.clear();
        check(changes == 1, "new SFI recomputes once");
        check(fresh.rows() != before, "new SFI changes the matrix within the hour");

        const auto before_kp = fresh.rows();
        changes = 0;
        kp_override = R"([{"time_tag":"2026-10-13T15:00:00","Kp":8.0,"a_running":207,"station_count":8}])";
        fresh.kpSource().refreshNow();
        for (int i = 0; i < 20; ++i) app.processEvents();
        kp_override.clear();
        check(changes == 1, "new Kp recomputes once");
        check(fresh.rows() != before_kp, "new Kp changes the matrix within the hour");

        // Age ticks in the same hour do no work, with or without a matrix.
        changes = 0;
        emit fresh.ageChanged();
        check(changes == 0, "age tick with a matrix does not recompute");
    }
    {
        // No DX target: nothing to rate, so an age tick must not re-read options and notify the UI.
        QTemporaryDir empty_dir;
        AppSettings no_dx(empty_dir.filePath("station.conf"), empty_dir.path());
        WidgetContext no_dx_context = context;
        no_dx_context.settings = &no_dx;
        BandsMatrixModel idle(no_dx_context);
        for (auto* source : {&idle.kpSource(), &idle.fluxSource()}) { source->setClock([&] { return now; }); QFile::remove(source->cachePath()); }
        idle.setClock([&] { return now; });
        idle.start();
        for (int i = 0; i < 20; ++i) app.processEvents();
        check(!idle.hasMatrix(), "no station, no matrix");
        int changes = 0;
        QObject::connect(&idle, &BandsMatrixModel::matrixChanged, &idle, [&] { ++changes; });
        emit idle.ageChanged();
        check(changes == 0, "age tick with no matrix does not recompute");
    }

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

    // The age of the values themselves, with both feeds answering on time.
    {
        struct Seen { QString state, status; bool has_matrix; QDateTime updated; };
        const auto seen = [&](double kp_hours, double sfi_hours) {
            kp_override = kp_row(now.addSecs(qint64(-kp_hours * 3600)));
            flux_override = flux_row(now.addSecs(qint64(-sfi_hours * 3600)));
            BandsMatrixModel aged(context);
            aged.setClock([&] { return now; });
            for (auto* source : {&aged.kpSource(), &aged.fluxSource()}) {
                source->setClock([&] { return now; });
                QFile::remove(source->cachePath());
            }
            aged.start();
            for (int i = 0; i < 20; ++i) app.processEvents();
            return Seen{aged.state(), aged.statusText(), aged.hasMatrix(), aged.updatedAt()};
        };
        check(seen(3.8, 4.8).state == "live", "Kp and SFI as the real feeds looked -> live");
        check(seen(8, 24).state == "live", "Kp row running late, SFI a day old -> live");
        check(seen(3, 73).state == "live", "SFI from Friday, on Monday -> live");
        const auto old_kp = seen(10, 20);
        check(old_kp.state == "stale", "old Kp, feed still answering -> stale");
        check(old_kp.status.contains("old Kp") && !old_kp.status.contains("SFI"), "note names only the old Kp");
        check(old_kp.has_matrix, "old Kp keeps the matrix on screen");
        check(old_kp.updated == now.addSecs(-10 * 3600), "age shown is the old reading's");
        const auto old_sfi = seen(3, 85);
        check(old_sfi.state == "stale", "SFI past a weekend, feed still answering -> stale");
        check(old_sfi.status.contains("old SFI") && !old_sfi.status.contains("Kp"), "note names only the old SFI");
        check(old_sfi.has_matrix, "old SFI keeps the matrix on screen");
        const auto both = seen(10, 85);
        check(both.state == "stale" && both.status.contains("SFI") && both.status.contains("Kp"), "both old -> both named");
        for (const auto& note : {old_kp.status, old_sfi.status, both.status})
            check(note.size() <= 28, "stale note fits the 800x480 footer");
        kp_override.clear();
        flux_override.clear();
    }

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
