// NOAA scales widget: parser and model, offline.
// Fixture is a real NOAA SWPC products/noaa-scales.json captured 2026-10-14.
#include "../solar_scales_model.hpp"
#include "../solar_scales_parse.hpp"
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
    const QByteArray body = file.readAll();

    const auto s = solarscales::parse_scales(body);
    check(s.valid, "fixture parses");
    check(s.observed == QDateTime(QDate(2026, 10, 14), QTime(2, 9), QTimeZone::utc()), "observed time is UTC");
    check(s.current.r == 0 && s.current.s == 0 && s.current.g == 0, "current all quiet");
    check(s.past24h.g == 0, "24 h max");
    check(s.forecast[0].r_minor == 10 && s.forecast[0].r_major == 1 && s.forecast[0].s_prob == 5, "day 1 probabilities");
    check(s.forecast[1].s_prob == 1 && s.forecast[2].g == 0, "later days");

    // Synthetic active state (not a captured sample): string numbers, as SWPC sends them.
    const auto active = solarscales::parse_scales(
        R"({"0":{"DateStamp":"2026-10-14","TimeStamp":"03:00:00","R":{"Scale":"2","Text":"moderate"},"S":{"Scale":"1"},"G":{"Scale":"4"}},)"
        R"("2":{"R":{"Scale":null,"MinorProb":"40","MajorProb":"10"},"S":{"Scale":null,"Prob":"15"},"G":{"Scale":"3","Text":"strong"}}})");
    check(active.valid && active.current.r == 2 && active.current.s == 1 && active.current.g == 4, "active levels");
    check(active.forecast[1].g == 3 && active.forecast[1].r_minor == 40 && active.forecast[1].s_prob == 15, "forecast values");
    check(solarscales::scale_word(4) == "Severe" && solarscales::scale_word(0).isEmpty() && solarscales::scale_word(9).isEmpty(),
          "scale words");

    check(!solarscales::parse_scales("").valid && !solarscales::parse_scales("<html>").valid &&
          !solarscales::parse_scales("[]").valid && !solarscales::parse_scales("{}").valid &&
          !solarscales::parse_scales(R"({"0":{"DateStamp":"2026-10-14","TimeStamp":"03:00:00","R":{"Scale":null}}})").valid &&
          !solarscales::parse_scales(R"({"0":{"R":{"Scale":"9"},"S":{"Scale":"0"},"G":{"Scale":"0"}}})").valid,
          "garbage, missing current state and out-of-range levels rejected");

    WidgetContext context;
    bool down = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult r;
        if (down) r.error = "network down"; else { r.http_status = 200; r.body = body; }
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    auto settle = [&] { for (int i = 0; i < 20; ++i) app.processEvents(); };
    auto now = QDateTime(QDate(2026, 10, 14), QTime(2, 30), QTimeZone::utc());
    SolarScalesModel model(context);
    model.setClock([&] { return now; });
    model.source().setClock([&] { return now; });
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading" && model.scales().isEmpty(), "loading before data");
    model.start();
    settle();
    check(model.state() == "live" && model.hasData(), "live");
    const auto rows = model.scales();
    check(rows.size() == 3 && rows[0].toMap()["letter"].toString() == "R" && rows[2].toMap()["letter"].toString() == "G", "R, S, G rows");
    check(rows[0].toMap()["outlook"].toString() == "R1-2 10%  R3+ 1%" && rows[1].toMap()["outlook"].toString() == "S1+ 1%" &&
          rows[2].toMap()["outlook"].toString() == "none expected", "outlook text for tomorrow");
    check(model.observed() == "02:09Z", "observed text");

    now = now.addSecs(4 * 3600);
    down = true;
    model.source().refreshNow();
    settle();
    check(model.state() == "stale" && model.hasData(), "outage -> stale, last values kept");

    SolarScalesModel offline(context);
    offline.setClock([&] { return now; });
    offline.source().setClock([&] { return now; });
    QFile::remove(offline.source().cachePath());
    offline.start();
    settle();
    check(offline.state() == "offline" && !offline.hasData(), "no data and down -> offline");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
