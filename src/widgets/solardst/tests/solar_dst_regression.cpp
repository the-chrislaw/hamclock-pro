// Dst widget: parser and model, offline.
// Fixture is a real NOAA SWPC products/kyoto-dst.json captured 2026-10-14.
#include "../solar_dst_model.hpp"
#include "../solar_dst_parse.hpp"
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
    QFile file(QDir(argv[1]).filePath("kyoto-dst.json"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray body = file.readAll();

    const auto s = solardst::parse_dst(body);
    check(s.size() == 167, "fixture record count");
    check(s.front().time == QDateTime(QDate(2026, 10, 7), QTime(4, 0), QTimeZone::utc()) && s.front().dst == -22, "first record");
    check(s.back().time == QDateTime(QDate(2026, 10, 14), QTime(2, 0), QTimeZone::utc()) && s.back().dst == -13, "last record");

    const auto odd = solardst::parse_dst(
        R"([{"time_tag":"2026-10-14T02:00:00","dst":-5},{"time_tag":"2026-10-14T01:00:00","dst":null},)"
        R"({"time_tag":"bad","dst":3},{"time_tag":"2026-10-14T00:00:00","dst":-7}])");
    check(odd.size() == 2 && odd[0].dst == -7 && odd[1].dst == -5, "bad records skipped, sorted oldest first");
    check(solardst::parse_dst("").empty() && solardst::parse_dst("<html>").empty() && solardst::parse_dst("{}").empty(),
          "garbage rejected");
    check(solardst::dst_word(-29) == "Quiet" && solardst::dst_word(-30) == "Weak" && solardst::dst_word(-50) == "Moderate" &&
          solardst::dst_word(-100) == "Intense" && solardst::dst_word(-251) == "Super" && solardst::dst_word(20) == "Quiet",
          "storm words");

    WidgetContext context;
    bool down = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult r;
        if (down) r.error = "network down"; else { r.http_status = 200; r.body = body; }
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    auto settle = [&] { for (int i = 0; i < 20; ++i) app.processEvents(); };
    auto now = QDateTime(QDate(2026, 10, 14), QTime(2, 30), QTimeZone::utc());
    SolarDstModel model(context);
    model.setClock([&] { return now; });
    model.source().setClock([&] { return now; });
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading" && model.history().isEmpty(), "loading before data");
    model.start();
    settle();
    check(model.state() == "live" && model.hasData(), "live");
    check(model.dst() == -13 && model.word() == "Quiet" && model.observed() == "02:00Z", "latest value");
    check(model.min24() == -13 || model.min24() < -13, "24 h minimum not above latest");
    check(model.history().size() == 49, "48 h history");

    now = now.addSecs(3 * 3600);
    down = true;
    model.source().refreshNow();
    settle();
    check(model.state() == "stale" && model.hasData(), "outage -> stale, last values kept");

    now = now.addSecs(3 * 3600);  // latest record now 7.5 h old (source recovered)
    down = false;
    model.source().refreshNow();
    settle();
    check(!model.hasData() && model.history().isEmpty(), "old index never shown as current");

    SolarDstModel offline(context);
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
