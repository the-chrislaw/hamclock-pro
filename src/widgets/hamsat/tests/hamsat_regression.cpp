// hams.at widget: parser and model, offline.
// Fixture is a trimmed real hams.at /api/alerts/upcoming response captured 2026-10-15.
#include "../hamsat_model.hpp"
#include "../hamsat_parse.hpp"
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

    const auto feed = hamsat::parse(body);
    check(feed.ok && feed.activations.size() == 5, "5 activations parsed");
    const auto& a0 = feed.activations.front();
    check(a0.callsign == "KB5FHK" && a0.satellite == "RS-44" && a0.mode == "SSB" && a0.mhz == 0 &&
          a0.aos == utc(2026, 10, 17, 1, 4).addSecs(40), "first row (no frequency announced)");
    check(feed.activations[2].mhz > 436.79 && feed.activations[2].mhz < 436.80, "frequency parsed");
    check(feed.activations.back().grids == "BL11 BL01", "multiple grids joined");
    check(!hamsat::parse("").ok && !hamsat::parse("{}").ok, "non-list rejected");
    check(hamsat::parse(R"({"data":[{"callsign":"","satellite":{"name":"x"},"aos_at":"2026-01-01T00:00:00Z","los_at":"2026-01-01T00:10:00Z"},)"
                        R"({"callsign":"A1B","satellite":{"name":"x"},"aos_at":"junk","los_at":"junk"}]})").activations.empty(),
          "malformed rows skipped");

    const auto now = utc(2026, 10, 17, 1, 0);
    check(hamsat::when_text(a0, now) == "in 4m", "minutes");
    check(hamsat::when_text(feed.activations[2], now) == "in 11h 58m", "hours");
    check(hamsat::when_text(feed.activations.back(), now) == "in 8d 1h", "days");
    check(hamsat::when_text(a0, utc(2026, 10, 17, 1, 10)) == "now", "under way");
    check(hamsat::upcoming(feed, utc(2026, 10, 17, 1, 30), 10).size() == 4, "finished pass dropped");
    check(hamsat::upcoming(feed, now, 2).size() == 2, "capped");

    WidgetContext context;
    bool fail_all = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = body; }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    HamsatModel model(context);
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
    check(model.nextTitle() == "KB5FHK on RS-44" && model.nextWhen() == "in 4m", "next activation");
    check(model.nextDetail() == "SSB · CM96 · 01:04Z-01:26Z", "detail line without frequency");
    check(model.later().size() == 3, "three later rows");

    fail_all = true;
    clock = clock.addSecs(8 * 3600);
    set_clock();
    model.source().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.hasNext(), "outage -> stale, last data kept");

    std::cout << "hams.at: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
