// Marine warnings widget: parser and model, offline.
// Fixture is a trimmed real api.weather.gov alerts response captured 2026-10-13 22:52Z.
#include "../marine_model.hpp"
#include "../marine_parse.hpp"
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
QByteArray fixture(const QString& dir, const char* name) {
    QFile file(QDir(dir).filePath(name));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
QDateTime utc(int d, int h, int m) { return QDateTime(QDate(2026, 10, d), QTime(h, m), QTimeZone::utc()); }
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    const QString dir = argv[1];
    const auto body = fixture(dir, "active");
    check(!body.isEmpty(), "fixture present");

    const auto feed = marine::parse(body);
    check(feed.ok && feed.alerts.size() == 6, "6 alerts parsed");
    check(feed.updated == QDateTime(QDate(2026, 10, 13), QTime(22, 52, 48), QTimeZone::utc()), "feed build time");
    check(feed.alerts[0].event == "Gale Warning" && feed.alerts[0].rank == 3, "gales first");
    // Alaska gale ends 2026-10-14 17:00 AKST = 02:00Z on the 15th; Eastport gales end 04:00 EDT = 08:00Z.
    check(feed.alerts[0].expires == utc(14, 8, 0) || feed.alerts[0].expires == utc(15, 2, 0), "expiry converted to UTC");
    check(feed.alerts[0].expires <= feed.alerts[1].expires && feed.alerts[1].expires <= feed.alerts[2].expires, "soonest expiry first within a rank");
    check(feed.alerts[3].event == "Small Craft Advisory" && feed.alerts[3].office.startsWith("NWS"), "advisories after warnings");
    check(marine::event_rank("Hurricane Force Wind Warning") == 5 && marine::event_rank("Flood Watch") == 0, "ranks");
    check(marine::requested_events().contains("Gale Warning,"), "event list");

    check(!marine::parse("").ok && !marine::parse("[]").ok && !marine::parse("{}").ok, "non-collection rejected");
    check(marine::parse(R"({"features":[]})").ok && marine::parse(R"({"features":[]})").alerts.empty(), "empty list is valid");
    const auto skipped = marine::parse(
        R"({"features":[{"properties":{"id":"","event":"Gale Warning","expires":"2026-10-14T00:00:00Z"}},
        {"properties":{"id":"a","event":"Flood Watch","expires":"2026-10-14T00:00:00Z"}},
        {"properties":{"id":"b","event":"Gale Warning"}}]})");
    check(skipped.ok && skipped.alerts.empty(), "malformed rows skipped");

    WidgetContext context;
    bool fail_all = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = body; }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    MarineModel model(context);
    auto now = utc(13, 23, 0);
    auto set_clock = [&] {
        model.setClock([&] { return now; });
        model.source().setClock([&] { return now; });
    };
    set_clock();
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading", "loading before data");
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasData() && model.count() == 6, "fresh -> live");
    check(model.headline() == "Gale Warning" && model.topRank() == 3, "headline");
    check(model.summary() == "3 Gale Warning · 3 Small Craft Advisory", "summary");
    check(model.headlineTimes().startsWith("issued ") && model.headlineTimes().contains("until ") && model.headlineTimes().endsWith("Z"), "times shown");

    // Past 04:00Z on the 14th the California advisories (and nothing else) have lapsed.
    now = utc(14, 4, 30);
    set_clock();
    emit model.ageChanged();
    check(model.count() == 4, "expired alerts dropped");

    fail_all = true;
    now = utc(14, 12, 0);
    set_clock();
    model.source().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.hasData(), "outage -> stale, last data kept");
    check(model.count() == 2, "alerts that ended at 08:00Z are gone even while stale");

    std::cout << "Marine: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
