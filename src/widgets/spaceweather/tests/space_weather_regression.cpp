// Space weather widget: SWPC parsers and model, offline.
// Fixtures are trimmed real NOAA SWPC responses captured 2026-10-13.
#include "../space_weather_model.hpp"
#include "../swpc_parse.hpp"
#include "../../../net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimeZone>
#include <QTimer>
#include <cmath>
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

    // Parsers on real data.
    const auto kp = swpc::latest_kp(fixture(dir, "noaa-planetary-k-index.json"));
    check(kp && kp->value == 3.0, "latest Kp from real SWPC product");
    check(kp && kp->time == QDateTime(QDate(2026, 10, 13), QTime(12, 0), QTimeZone::utc()), "Kp time is UTC");
    const auto sfi = swpc::latest_solar_flux(fixture(dir, "10cm-flux.json"));
    check(sfi && sfi->value == 123, "solar flux from summary product");
    const auto xray = swpc::latest_xray_flux(fixture(dir, "xrays-6-hour.json"));
    check(xray && std::abs(xray->value - 7.258945515786763e-07) < 1e-15, "X-ray uses the 0.1-0.8 nm band only");
    check(xray && swpc::xray_class(xray->value) == "B7.2", "real flux classifies as B7.2");

    // 24 h series: bucket maximum, 0.1-0.8 nm band only, oldest first.
    const auto day = swpc::xray_series(fixture(dir, "xrays-1-day.json"), 10);
    check(day.size() >= 140 && day.size() <= 145, "24 h of 10-minute points");
    check(day.size() > 1 && day.front().time < day.back().time, "series is oldest first");
    check(swpc::xray_series("[{\"time_tag\":\"2026-10-13T00:01:00Z\",\"flux\":1e-6,\"energy\":\"0.1-0.8nm\"},"
                            "{\"time_tag\":\"2026-10-13T00:04:00Z\",\"flux\":3e-6,\"energy\":\"0.1-0.8nm\"},"
                            "{\"time_tag\":\"2026-10-13T00:05:00Z\",\"flux\":9e-5,\"energy\":\"0.05-0.4nm\"}]", 10).size() == 1,
          "short band ignored, one bucket");
    check(swpc::xray_series("[{\"time_tag\":\"2026-10-13T00:01:00Z\",\"flux\":1e-6,\"energy\":\"0.1-0.8nm\"},"
                            "{\"time_tag\":\"2026-10-13T00:04:00Z\",\"flux\":3e-6,\"energy\":\"0.1-0.8nm\"}]", 10)[0].value == 3e-6,
          "bucket keeps the peak");
    check(swpc::xray_series("garbage", 10).empty() && swpc::xray_series("[]", 0).empty(), "bad input -> empty series");
    check(swpc::xray_plot_fraction(1e-8) == 0 && swpc::xray_plot_fraction(0) == 0 &&
          std::abs(swpc::xray_plot_fraction(1e-5) - 0.6) < 1e-9 && swpc::xray_plot_fraction(1) == 1,
          "plot axis A1..X10 clamped");

    // Malformed and hostile input never yields a value.
    for (const char* bad : {"", "{}", "[", "null", "[{\"time_tag\":\"x\",\"Kp\":3}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"Kp\":12}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"Kp\":\"3\"}]"})
        check(!swpc::latest_kp(bad), "malformed Kp rejected");
    check(!swpc::latest_xray_flux("[{\"time_tag\":\"2026-10-13T00:00:00Z\",\"flux\":1e-6,\"energy\":\"0.05-0.4nm\"}]"),
          "short-wavelength band ignored");
    check(swpc::latest_kp("[{\"time_tag\":\"2026-10-13T03:00:00\",\"Kp\":2},{\"time_tag\":\"2026-10-13T00:00:00\",\"Kp\":7}]")->value == 2,
          "latest by time, not by position");

    // Flare classes at band edges (GOES scale).
    check(swpc::xray_class(1e-8) == "A1.0" && swpc::xray_class(9.99e-8) == "A9.9", "A class");
    check(swpc::xray_class(1e-7) == "B1.0" && swpc::xray_class(3.2e-6) == "C3.2", "B and C class");
    check(swpc::xray_class(5.5e-5) == "M5.5" && swpc::xray_class(1e-4) == "X1.0", "M and X class");
    check(swpc::xray_class(1.2e-3) == "X12", "X10+ written as integer");
    check(swpc::xray_class(0).isEmpty() && swpc::xray_class(NAN).isEmpty(), "invalid flux has no class");
    check(swpc::kp_description(3) == "Quiet" && swpc::kp_description(4.33) == "Active" &&
          swpc::kp_description(5) == "Minor storm (G1)" && swpc::kp_description(9) == "Extreme storm (G5)",
          "NOAA G-scale descriptions");

    // Model end to end with a fixture transport.
    WidgetContext context;
    int requests = 0;
    bool fail_all = false;
    context.transport = [&](const QUrl& url, FetchDone done) {
        ++requests;
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = fixture(dir, QFileInfo(url.path()).fileName().toUtf8().constData()); }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    SpaceWeatherModel model(context);
    const auto now = QDateTime(QDate(2026, 10, 13), QTime(16, 0), QTimeZone::utc());
    model.setClock([&] { return now; });
    for (auto* source : {&model.kpSource(), &model.fluxSource(), &model.xraySource(), &model.xrayDaySource()})
        source->setClock([&] { return now; });
    // Start from an empty cache so every run exercises the network path, and
    // let all startup requests finish before simulating an outage.
    for (auto* source : {&model.kpSource(), &model.fluxSource(), &model.xraySource(), &model.xrayDaySource()})
        QFile::remove(source->cachePath());
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(requests == 4, "four SWPC requests on start");
    check(model.state() == "live", "all fresh sources -> live");
    check(model.hasKp() && model.kp() == 3.0 && model.kpText() == "Quiet", "model Kp");
    check(model.hasSolarFlux() && model.solarFlux() == 123, "model solar flux");
    check(model.xrayClass() == "B7.2", "model X-ray class");
    check(model.hasXrayHistory() && model.xrayHistory().size() == int(day.size()), "model exposes 24 h history");

    // Network loss keeps the last good values but reports them as stale.
    fail_all = true;
    model.setClock([&] { return now.addSecs(5 * 3600); });
    for (auto* source : {&model.kpSource(), &model.fluxSource(), &model.xraySource(), &model.xrayDaySource()}) {
        source->setClock([&] { return now.addSecs(5 * 3600); });
        source->refreshNow();
    }
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale", "old data after failures -> stale, not live");
    check(model.hasKp() && model.kp() == 3.0, "last good value kept while stale");
    check(!model.hasXrayHistory(), "stale history is not plotted");
    check(model.statusText().contains("Kp"), "status names the stale source");

    std::cout << "Space weather: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
