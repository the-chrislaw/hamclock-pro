// hams.at widget: parser and model, offline.
// Fixture is a trimmed real hams.at /api/alerts/upcoming response captured 2026-10-15.
#include "../hamsat_model.hpp"
#include "../hamsat_parse.hpp"
#include "../../../net/polled_source.hpp"
#include "../../../ui/app_settings.hpp"
#include <QTemporaryDir>
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

    // Personalised list: authenticated request carries the key; public fallback on 401.
    QFile fa(QDir(argv[1]).filePath("upcoming_authed"));
    check(fa.open(QIODevice::ReadOnly), "authed fixture present");
    const QByteArray authed_body = fa.readAll();
    const auto pf = hamsat::parse(authed_body);
    check(pf.ok && pf.activations.size() == 2 && pf.activations[0].workable == 1 &&
          pf.activations[0].max_elevation > 42.3 && pf.activations[0].max_elevation < 42.5 &&
          pf.activations[0].workable_start == utc(2026, 10, 17, 1, 12) &&
          pf.activations[1].workable == 0 && !pf.activations[1].workable_start.isValid(),
          "per-station fields parsed");
    check(feed.activations[0].workable == 0 && feed.activations[0].max_elevation < 0, "public list has no elevation");

    QTemporaryDir dir;
    AppSettings settings({}, dir.path());
    check(!settings.hasHamsatKey(), "no key initially");
    check(!settings.saveHamsatKey("abc def").isEmpty() && !settings.hasHamsatKey(), "key with space rejected");
    QByteArray seen_bearer = "unset";
    bool reject = false;
    int calls = 0;
    WidgetContext authed;
    authed.settings = &settings;
    authed.transport = context.transport;
    authed.auth_transport = [&](const QUrl&, const QByteArray& bearer, FetchDone done) {
        ++calls;
        seen_bearer = bearer;
        FetchResult result;
        if (reject && !bearer.isEmpty()) { result.http_status = 401; result.error = {}; }
        else { result.http_status = 200; result.body = bearer.isEmpty() ? body : authed_body; }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    HamsatModel m2(authed);
    auto clock2 = now;
    m2.setClock([&] { return clock2; });
    m2.source().setClock([&] { return clock2; });
    QFile::remove(m2.source().cachePath());
    m2.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(seen_bearer.isEmpty() && !m2.personalised() && m2.nextTitle() == "KB5FHK on RS-44", "no key -> public request");
    check(settings.saveHamsatKey("  secret-key-1  ").isEmpty() && settings.hasHamsatKey(), "key saved");
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(seen_bearer == "secret-key-1" && m2.personalised(), "key change refetches with bearer");
    check(m2.nextDetail() == "FM · 436.795 MHz · EN34 · 01:10Z-01:22Z · workable 01:12Z-01:20Z max el 42°",
          "workable window and elevation shown");
    check(m2.later().size() == 0 || m2.later().size() == 1, "later rows from personalised list");
    reject = true;
    calls = 0;
    m2.source().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(calls == 2 && seen_bearer.isEmpty() && !m2.personalised() && m2.hasNext(), "401 -> public list, key not sent again");
    check(m2.statusText().contains("rejected"), "rejected key explained");
    check(settings.saveHamsatKey("").isEmpty() && !settings.hasHamsatKey(), "key cleared");
    for (int i = 0; i < 20; ++i) app.processEvents();

    std::cout << "hams.at: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
