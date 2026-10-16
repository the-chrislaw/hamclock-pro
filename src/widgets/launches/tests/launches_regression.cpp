// Launches widget: parser and model, offline.
// Fixture is a trimmed real Launch Library 2 upcoming list captured 2026-10-14.
#include "../launches_model.hpp"
#include "../launches_parse.hpp"
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
QDateTime utc(int y, int mo, int d, int h, int mi) {
    return QDateTime(QDate(y, mo, d), QTime(h, mi), QTimeZone::utc());
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    QFile f(QDir(argv[1]).filePath("upcoming"));
    check(f.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray body = f.readAll();

    const auto feed = launches::parse(body);
    check(feed.ok && feed.launches.size() == 6, "6 launches parsed");
    check(feed.launches.front().name.startsWith("Zhuque-2E") && feed.launches.front().provider == "LandSpace", "first row");
    check(feed.launches[1].net == utc(2026, 10, 15, 23, 44) && feed.launches[1].status == "Go" &&
          feed.launches[1].precise, "second row time and status");
    check(!feed.launches.back().precise && feed.launches.back().status == "TBD", "date-only launch is approximate");
    check(launches::ham_hint(feed.launches[2]).startsWith("Rideshare"), "Bandwagon rideshare hinted");
    check(launches::ham_hint(feed.launches[3]).isEmpty() && launches::ham_hint(feed.launches[4]).isEmpty(),
          "Starlink and MMX not hinted");
    launches::Launch iss; iss.name = "Falcon 9 | Crew-14";
    check(launches::ham_hint(iss) == "ISS visit (ARISS)", "crew flight hinted");
    launches::Launch cube; cube.name = "Electron | Amateur radio cubesat"; 
    check(launches::ham_hint(cube) == "Amateur/cubesat payloads", "amateur payload hinted");
    check(!launches::parse("").ok && !launches::parse("{}").ok, "non-list rejected");
    check(launches::parse(R"({"results":[{"name":"","net":"2026-01-01T00:00:00Z"},{"name":"x","net":"junk"}]})").launches.empty(),
          "malformed rows skipped");

    const auto now = utc(2026, 10, 14, 20, 0);
    check(launches::countdown_text(utc(2026, 10, 15, 23, 44), now) == "T-27h 44m", "countdown hours");
    check(launches::countdown_text(utc(2026, 10, 19, 10, 30), now) == "T-4d 14h", "countdown days");
    check(launches::countdown_text(utc(2026, 10, 14, 20, 12), now) == "T-12m", "countdown minutes");
    check(launches::countdown_text(utc(2026, 10, 14, 19, 0), now) == "T+0m", "past launch");
    const auto up = launches::upcoming(feed, now, 3);
    check(up.size() == 3 && up.front().net == utc(2026, 10, 15, 23, 44), "past launch dropped, capped");

    WidgetContext context;
    bool fail_all = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = body; }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    LaunchesModel model(context);
    auto clock = now;
    auto set_clock = [&] {
        model.setClock([&] { return clock; });
        model.source().setClock([&] { return clock; });
    };
    set_clock();
    QFile::remove(model.source().cachePath());
    check(model.state() == "loading" && !model.hasNext(), "loading before data");
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasNext(), "fresh -> live");
    check(model.nextName().contains("USSF-481") && model.nextCountdown() == "T-27h 44m", "next launch");
    check(model.nextDetail() == "SpaceX · Vandenberg SFB, CA, USA · Go", "detail line");
    check(model.later().size() == 3, "three later rows");

    fail_all = true;
    clock = clock.addSecs(8 * 3600);
    set_clock();
    model.source().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.hasNext(), "outage -> stale, last data kept");

    std::cout << "Launches: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
