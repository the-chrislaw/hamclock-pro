// Firewx warnings widget: parser and model, offline.
// Fixture is a real api.weather.gov fire-weather alerts response captured 2026-10-13 23:51Z (one Fire Weather Watch was active).
#include "../firewx_model.hpp"
#include "../firewx_parse.hpp"
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

    const auto feed = firewx::parse(body);
    check(feed.ok && feed.alerts.size() == 1, "1 alert parsed");
    check(feed.updated == QDateTime(QDate(2026, 10, 13), QTime(23, 50, 56), QTimeZone::utc()), "feed build time");
    check(feed.alerts[0].event == "Fire Weather Watch" && feed.alerts[0].rank == 3, "watch rank");
    check(feed.alerts[0].area == "Tehachapi Area; Fort Tejon" && feed.alerts[0].office == "NWS Hanford CA", "area and office");
    // "ends" (16 Oct 22:00 PDT = 17 Oct 05:00Z) wins over the earlier "expires".
    check(feed.alerts[0].expires == utc(17, 5, 0), "ends preferred, converted to UTC");
    check(feed.alerts[0].onset == utc(15, 13, 0), "onset converted to UTC");
    check(firewx::event_rank("Red Flag Warning") == 4 && firewx::event_rank("Gale Warning") == 0, "ranks");
    check(firewx::requested_events().contains("Red Flag Warning,"), "event list");

    check(!firewx::parse("").ok && !firewx::parse("[]").ok && !firewx::parse("{}").ok, "non-collection rejected");
    check(firewx::parse(R"({"features":[]})").ok && firewx::parse(R"({"features":[]})").alerts.empty(), "empty list is valid");
    const auto skipped = firewx::parse(
        R"({"features":[{"properties":{"id":"","event":"Red Flag Warning","expires":"2026-10-14T00:00:00Z"}},
        {"properties":{"id":"a","event":"Flood Watch","expires":"2026-10-14T00:00:00Z"}},
        {"properties":{"id":"b","event":"Red Flag Warning"}}]})");
    check(skipped.ok && skipped.alerts.empty(), "malformed rows skipped");
    const auto order = firewx::parse(
        R"({"features":[{"properties":{"id":"w","event":"Fire Weather Watch","expires":"2026-10-14T00:00:00Z"}},
        {"properties":{"id":"r","event":"Red Flag Warning","expires":"2026-10-15T00:00:00Z"}}]})");
    check(order.alerts.size() == 2 && order.alerts[0].event == "Red Flag Warning", "warnings sort before watches");

    QTemporaryDir tmp;
    AppSettings settings(tmp.filePath("station.conf"), tmp.path());
    WidgetContext context;
    context.settings = &settings;
    bool fail_all = false;
    QString last_point_url;
    QByteArray point_body = fixture(dir, "point-empty.json"); // real capture 2026-10-14 05:45Z, Denver
    check(!point_body.isEmpty(), "point fixture present");
    context.transport = [&](const QUrl& url, FetchDone done) {
        FetchResult result;
        const bool is_point = url.query().contains("point=");
        if (is_point) last_point_url = url.toString();
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = is_point ? point_body : body; }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    FirewxModel model(context);
    auto now = utc(13, 23, 55);
    auto set_clock = [&] {
        model.setClock([&] { return now; });
        model.source().setClock([&] { return now; });
    };
    set_clock();
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading", "loading before data");
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasData() && model.count() == 1, "fresh -> live");
    check(model.headline() == "Fire Weather Watch" && model.topRank() == 3, "headline");
    check(model.shortHeadline() == "Fire Watch", "short headline");
    check(model.summary() == "1 Fire Weather Watch", "summary");
    check(model.headlineTimes().startsWith("issued 13 16:50Z · until 17 05:00Z"), "times shown");
    check(model.headlineStart() == "starts 15 13:00Z", "start shown while ahead");

    check(model.atDe().isEmpty() && !model.pointSource(), "no station -> no point query");
    check(settings.saveStation("W1AW", 41.7, -72.7, 51.5, -0.1, "").isEmpty(), "station saved");
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.pointSource() != nullptr, "station -> point source");
    check(last_point_url.contains("point=41.7000,-72.7000"), "point query uses DE");
    model.pointSource()->setClock([&] { return now; });
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.atDe() == "At DE: none active", "empty point response -> none active");
    point_body = body; // a real alert body as the point answer
    model.pointSource()->refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.atDe() == "At DE: Fire Watch · until 17 05:00Z", "alert at DE shown with expiry");

    now = utc(15, 14, 0);
    set_clock();
    emit model.ageChanged();
    check(model.headlineStart().isEmpty(), "start hidden once the watch is in effect");

    fail_all = true;
    now = utc(16, 12, 0);
    set_clock();
    model.source().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.hasData() && model.count() == 1, "outage -> stale, last data kept");

    check(model.atDe().isEmpty(), "stale point data is not shown as 'none active'");

    now = utc(17, 6, 0);
    set_clock();
    emit model.ageChanged();
    check(model.count() == 0, "expired alert dropped even while stale");

    std::cout << "Firewx: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
