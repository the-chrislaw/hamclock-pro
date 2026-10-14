// D-RAP widget: parser, map style and model, offline.
// Fixture is a real NOAA SWPC text/drap_global_frequencies.txt captured 2026-10-14 06:07 UTC.
#include "../solar_drap_model.hpp"
#include "../solar_drap_parse.hpp"
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
    QFile file(QDir(argv[1]).filePath("drap_global_frequencies.txt"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray body = file.readAll();

    const auto g = solardrap::parse_drap(body);
    check(g.valid && g.valid_at == QDateTime(QDate(2026, 10, 14), QTime(6, 7), QTimeZone::utc()), "valid time");
    check(g.xray == "Normal X-ray Background" && g.proton == "Normal Proton Background", "messages");
    check(g.mhz.size() == 90u * 90u, "full grid");
    check(solardrap::peak(g) > 3.9 && solardrap::peak(g) < 4.1, "peak 4.0 MHz");
    check(g.mhz[0] == 0.0f, "north pole row is zero");

    const auto img = solardrap::to_image(g);
    check(img.size() == QSize(360, 180), "image size");
    // Fixture absorption sits near the subsolar region around 0 N; the image must have some opaque pixels.
    int opaque = 0;
    for (int y = 0; y < 180; ++y) for (int x = 0; x < 360; ++x) opaque += img.pixelColor(x, y).alpha() > 0;
    check(opaque > 0 && img.pixelColor(0, 0).alpha() == 0, "image has coloured cells, pole transparent");

    QByteArray cut = body;
    cut.truncate(cut.size() / 2);
    check(!solardrap::parse_drap(cut).valid && !solardrap::parse_drap("").valid &&
          !solardrap::parse_drap("<html>").valid, "truncated or garbage rejected");

    WidgetContext context;
    bool down = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult r;
        if (down) r.error = "network down"; else { r.http_status = 200; r.body = body; }
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    auto settle = [&] { for (int i = 0; i < 20; ++i) app.processEvents(); };
    auto now = QDateTime(QDate(2026, 10, 14), QTime(6, 20), QTimeZone::utc());
    auto has_style = [] {
        for (const auto& s : MapStyles::instance().styles()) if (s.id == "drap") return !s.image.isNull();
        return false;
    };
    SolarDrapModel model(context);
    model.setClock([&] { return now; });
    model.source().setClock([&] { return now; });
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading" && !has_style(), "loading before data");
    model.start();
    settle();
    check(model.state() == "live" && model.hasData() && model.observed() == "06:07Z", "live");
    check(model.peakMhz() > 3.9 && has_style(), "D-RAP map style published");
    check(model.message().contains("Normal X-ray Background"), "message");

    now = now.addSecs(4500);
    down = true;
    model.source().refreshNow();
    settle();
    check(model.state() == "stale" && model.hasData() && has_style(), "outage within 2 h -> stale, values kept");

    now = now.addSecs(3600);  // table now 2 h 38 min old
    down = false;
    model.source().refreshNow();
    settle();
    check(!model.hasData() && !has_style(), "old table never shown or drawn as current");

    SolarDrapModel offline(context);
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
