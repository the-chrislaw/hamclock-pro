// Solar wind widget: parsers and model, offline.
// Fixtures are trimmed real NOAA SWPC RTSW responses captured 2026-10-13.
#include "../solar_wind_model.hpp"
#include "../solar_wind_parse.hpp"
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
    const auto wind = fixture(dir, "rtsw_wind_1m.json");
    const auto mag = fixture(dir, "rtsw_mag_1m.json");
    check(!wind.isEmpty() && !mag.isEmpty(), "fixtures present");

    // Only the active spacecraft is used, oldest first, one point per minute.
    const auto speed = solarwind::series(wind, solarwind::kSpeed);
    check(!speed.empty(), "speed series parsed");
    check(speed.size() == 10, "10 active SOLAR1 rows of the 24 (ACE backup rows ignored)");
    bool sorted = true;
    for (size_t i = 1; i < speed.size(); ++i) sorted = sorted && speed[i - 1].time < speed[i].time;
    check(sorted, "chronological, no duplicate minutes");
    check(speed.back().time == QDateTime(QDate(2026, 10, 13), QTime(17, 38), QTimeZone::utc()), "latest time is UTC");
    check(speed.back().value == 565.6, "latest active speed (not the ACE 610.35)");
    const auto bz = solarwind::series(mag, solarwind::kBz);
    check(!bz.empty() && bz.back().value == 1.42, "latest Bz is GSM component");
    check(!solarwind::series(mag, solarwind::kBt).empty(), "Bt parsed");
    check(!solarwind::series(wind, solarwind::kDensity).empty(), "density parsed");

    // Hostile and degenerate input yields nothing rather than a value.
    for (const char* bad : {"", "{}", "[", "null", "[1,2]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"active\":true,\"bt\":null}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"active\":true,\"bt\":\"5\"}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"active\":true,\"bt\":-9999}]",
                            "[{\"time_tag\":\"bogus\",\"active\":true,\"bt\":5}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"active\":false,\"bt\":5}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"bt\":5}]"})
        check(solarwind::series(bad, solarwind::kBt).empty(), "malformed row rejected");
    check(solarwind::speed_description(350) == "Slow" && solarwind::speed_description(450) == "Moderate" &&
          solarwind::speed_description(650) == "Fast" && solarwind::speed_description(900) == "Very fast",
          "speed descriptions");
    check(solarwind::bz_description(2) == "Northward" && solarwind::bz_description(-1) == "Near neutral" &&
          solarwind::bz_description(-5) == "Weakly south" && solarwind::bz_description(-10) == "Southward" &&
          solarwind::bz_description(-20) == "Strongly south", "Bz descriptions");

    // Model end to end with a fixture transport.
    WidgetContext context;
    bool fail_all = false;
    int requests = 0;
    context.transport = [&](const QUrl& url, FetchDone done) {
        ++requests;
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = fixture(dir, QFileInfo(url.path()).fileName().toUtf8().constData()); }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    SolarWindModel model(context);
    auto now = QDateTime(QDate(2026, 10, 13), QTime(17, 40), QTimeZone::utc());
    auto set_clock = [&] {
        model.setClock([&] { return now; });
        for (auto* s : {&model.windSource(), &model.magSource()}) s->setClock([&] { return now; });
    };
    set_clock();
    for (auto* s : {&model.windSource(), &model.magSource()}) QFile::remove(s->cachePath());
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(requests == 2, "two SWPC requests on start");
    check(model.state() == "live", "fresh data -> live");
    check(model.hasSpeed() && model.speed() == 565.6 && model.speedText() == "Moderate", "model speed");
    check(model.hasBz() && model.bz() == 1.42 && model.bzText() == "Northward" && model.hasBt(), "model Bz/Bt");
    check(model.speedHistory().size() == 10 && model.bzHistory().size() > 2, "history exposed for sparklines");
    check(model.updatedAt() == speed.back().time || model.updatedAt() >= speed.back().time.addSecs(-120),
          "updatedAt is the observation time");

    // Fresh download but old spacecraft data must not read as live.
    now = now.addSecs(40 * 60);
    set_clock();
    model.windSource().refreshNow();
    model.magSource().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.statusText().contains("old"), "old observation -> stale");

    // Outage keeps last values but marks stale.
    fail_all = true;
    now = now.addSecs(3600);
    set_clock();
    model.windSource().refreshNow();
    model.magSource().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.hasSpeed(), "outage -> stale with last value kept");

    std::cout << "Solar wind: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
