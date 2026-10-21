// Balloons widget: parser and model, offline.
// Fixture is a trimmed real SondeHub telemetry response captured 2026-10-20
// (six sondes, newest two frames each, fields reduced to those we read).
#include "../balloons_model.hpp"
#include "../balloons_parse.hpp"
#include "../../../net/polled_source.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
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
    QFile f(QDir(argv[1]).filePath("telemetry"));
    check(f.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray body = f.readAll();

    const auto feed = balloons::parse(body);
    check(feed.ok && feed.sondes.size() == 6, "six sondes, one frame each");
    const balloons::Sonde* v = nullptr;
    for (const auto& s : feed.sondes) if (s.serial == "V1710894") v = &s;
    check(v && v->type == "RS41" && v->time == utc(2026, 10, 20, 21, 25).addSecs(28) && qAbs(v->lat - 40.05623) < 1e-4,
          "newest frame kept");
    check(v && balloons::phase(*v) == "climbing", "positive vertical speed climbs");
    balloons::Sonde down; down.has_vel_v = true; down.vel_v = -12;
    balloons::Sonde level; level.has_vel_v = true; level.vel_v = 0.3;
    check(balloons::phase(down) == "descending" && balloons::phase(level).isEmpty() &&
          balloons::phase(balloons::Sonde{}).isEmpty(), "descending / level / unknown");
    {
        balloons::Sonde d; d.has_vel_v = true; d.alt_m = 10000; d.vel_v = -20;
        const int eta = balloons::landing_eta_s(d);
        check(eta > 690 && eta < 730, "10 km at 20 m/s: about 12 min (hand-computed 711 s)");
        d.alt_m = 0; check(balloons::landing_eta_s(d) == 0, "on the ground: 0 s");
        d.alt_m = 10000; d.vel_v = -1; check(balloons::landing_eta_s(d) == -1, "slow descent: unknown");
        d.vel_v = 5; check(balloons::landing_eta_s(d) == -1, "climbing: none");
        d.has_vel_v = false; check(balloons::landing_eta_s(d) == -1, "no vertical speed: none");
        d.has_vel_v = true; d.vel_v = -20;
        const int left = balloons::landing_eta_remaining_s(d, 300);
        check(left > 390 && left < 430, "report 5 min old: 711 s - 300 s");
        check(balloons::landing_eta_remaining_s(d, 1500) == 0, "report 25 min old: already elapsed");
        d.vel_v = 5; check(balloons::landing_eta_remaining_s(d, 10) == -1, "climbing stays unknown");
    }
    check(!balloons::parse("").ok && !balloons::parse("[]").ok, "non-object rejected");
    check(balloons::parse(R"({"A":{"t":{"datetime":"junk","lat":1,"lon":2}},"B":{"t":{"datetime":"2026-01-01T00:00:00Z","lat":95,"lon":2}}})")
              .sondes.empty(), "bad time or position skipped");

    const auto now = utc(2026, 10, 20, 21, 30);
    const auto near = balloons::nearest(feed, 39.74, -104.99, now, 1800, 3);
    check(near.size() == 3 && near.front().sonde.serial == "V1710894", "Denver: nearest is the Colorado sonde");
    check(near.front().distance_km > 40 && near.front().distance_km < 60 && near.front().bearing_deg > 45 && near.front().bearing_deg < 90,
          "distance and bearing from DE");
    check(near[0].distance_km <= near[1].distance_km && near[1].distance_km <= near[2].distance_km, "sorted by distance");
    check(balloons::nearest(feed, 39.74, -104.99, utc(2026, 10, 20, 23, 0), 1800, 3).empty(), "old frames dropped");

    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());
    WidgetContext context;
    context.settings = &settings;
    bool fail_all = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = body; }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    BalloonsModel model(context);
    auto clock = now;
    auto set_clock = [&] {
        model.setClock([&] { return clock; });
        model.source().setClock([&] { return clock; });
    };
    set_clock();
    QFile::remove(model.source().cachePath());
    check(model.state() == "loading" && model.nearest().isEmpty(), "loading before data");
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "live" && model.hasData() && model.count() == 6, "fresh -> live, six heard");
    check(model.nearest().isEmpty() && !model.hasStation(), "no station: no nearest");
    check(settings.saveStation("W0TST", 39.74, -104.99, 0, 0, "").isEmpty(), "station saved");
    check(model.nearest().endsWith("NE") || model.nearest().endsWith("E"), "nearest is north-east/east of Denver");
    check(model.nearestDetail().startsWith("RS41 V1710894 · 18.5 km up · climbing"), "detail line");
    check(model.later().size() == 3, "three later rows");
    {
        bool found = false;
        for (const auto& l : MapOverlays::instance().layers())
            if (l.id == "balloons") found = l.markers.size() == 6;
        check(found, "sondes on the map");
    }

    fail_all = true;
    clock = clock.addSecs(2 * 3600);
    set_clock();
    model.source().refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.count() == 0, "outage -> stale, old sondes not counted as flying");
    {
        bool found = false;
        for (const auto& l : MapOverlays::instance().layers()) if (l.id == "balloons") found = true;
        check(!found, "stale data removes the markers");
    }
    std::cout << "Balloons: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
