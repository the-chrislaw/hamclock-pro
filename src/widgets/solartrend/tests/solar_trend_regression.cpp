// Solar trend widget: parsers and model, offline.
// Fixtures are trimmed real NOAA SWPC responses captured 2026-10-13.
#include "../solar_trend_model.hpp"
#include "../solar_trend_parse.hpp"
#include "../../../net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <vector>
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
    const auto kp = fixture(dir, "noaa-planetary-k-index-forecast.json");
    const auto flux = fixture(dir, "f107_cm_flux.json");
    const auto ssn = fixture(dir, "observed-solar-cycle-indices.json");
    check(!kp.isEmpty() && !flux.isEmpty() && !ssn.isEmpty(), "fixtures present");

    // Kp bars: window starts 36 h before the newest observation, forecast kept.
    const auto bars = solartrend::kp_bars(kp);
    check(!bars.empty(), "kp bars parsed");
    bool sorted = true, seen_predicted = false, predicted_tail = true;
    for (size_t i = 1; i < bars.size(); ++i) sorted = sorted && bars[i - 1].time < bars[i].time;
    for (const auto& b : bars) { seen_predicted = seen_predicted || b.predicted; if (seen_predicted && !b.predicted) predicted_tail = false; }
    check(sorted, "kp bars chronological");
    check(seen_predicted && predicted_tail, "forecast bars come last");
    QDateTime newest_real;
    for (const auto& b : bars) if (!b.predicted) newest_real = b.time;
    check(bars.front().time >= newest_real.addSecs(-36 * 3600) && bars.front().time <= newest_real, "36 h of history");
    check(bars.size() <= 32, "bounded bar count");
    check(solartrend::kp_bars("[{\"time_tag\":\"2026-10-16T00:00:00\",\"kp\":2,\"observed\":\"predicted\"}]").empty(),
          "forecast alone is not shown");
    check(solartrend::kp_level(3.67) == 0 && solartrend::kp_level(4) == 1 && solartrend::kp_level(5) == 2, "kp levels");

    // Flux: one Noon reading per UTC day, at most 30 days.
    const auto days = solartrend::flux_daily(flux);
    check(days.size() == 30, "30 days of flux (fixture has 41)");
    bool days_sorted = true;
    for (size_t i = 1; i < days.size(); ++i) days_sorted = days_sorted && days[i - 1].date < days[i].date;
    check(days_sorted, "flux oldest first, one per day");
    check(days.back().date == QDate(2026, 10, 12) && days.back().flux == 123.0, "newest Noon reading is Oct 12, 123 sfu");
    check(solartrend::flux_daily(flux, 5).size() == 5, "limit honoured");

    solartrend::Sunspots s;
    check(solartrend::latest_sunspots(ssn, &s) && s.month == "2026-09" && s.number == 68.73, "latest NOAA monthly SSN");

    // Hostile input yields nothing.
    for (const char* bad : {"", "{}", "[", "null", "[1,2]"}) {
        check(solartrend::kp_bars(bad).empty() && solartrend::flux_daily(bad).empty() &&
              !solartrend::latest_sunspots(bad, &s), "malformed body rejected");
    }
    check(solartrend::flux_daily("[{\"time_tag\":\"2026-10-12T20:00:00\",\"flux\":-1,\"reporting_schedule\":\"Noon\"},"
                                 "{\"time_tag\":\"2026-10-12T20:00:00\",\"flux\":\"9\",\"reporting_schedule\":\"Noon\"},"
                                 "{\"time_tag\":\"2026-10-12T17:00:00\",\"flux\":99,\"reporting_schedule\":\"Morning\"}]").empty(),
          "bad flux values and non-Noon rows rejected");
    check(!solartrend::latest_sunspots("[{\"time-tag\":\"2026-09\",\"observed_swpc_ssn\":-1.0}]", &s),
          "-1 missing SSN rejected");
    check(solartrend::kp_bars("[{\"time_tag\":\"2026-10-12T00:00:00\",\"kp\":12,\"observed\":\"observed\"}]").empty(),
          "Kp above 9 rejected");

    // Model end to end with a fixture transport.
    WidgetContext context;
    bool fail_all = false;
    int requests = 0;
    // Answers for URLs ending in one of these are held back, as when one
    // product is slow at startup while the others have arrived.
    QStringList hold;
    std::vector<std::pair<FetchDone, QString>> held;
    context.transport = [&](const QUrl& url, FetchDone done) {
        ++requests;
        if (std::any_of(hold.begin(), hold.end(), [&](const QString& h) { return url.path().endsWith(h); })) {
            held.emplace_back(done, QFileInfo(url.path()).fileName());
            return;
        }
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = fixture(dir, QFileInfo(url.path()).fileName().toUtf8().constData()); }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    QDateTime now(QDate(2026, 10, 13), QTime(18, 0), QTimeZone::utc());
    {
        hold = {"observed-solar-cycle-indices.json"};
        SolarTrendModel early(context);
        early.setClock([&] { return now; });
        for (auto* src : {&early.kpSource(), &early.fluxSource(), &early.sunspotSource()}) {
            src->setClock([&] { return now; });
            QFile::remove(src->cachePath());
        }
        early.start();
        for (int i = 0; i < 20; ++i) app.processEvents();
        check(early.state() == "loading" && !early.statusText().contains("missing"),
              "sunspots still in flight -> loading, not missing");
        hold.clear();
        held.clear();
        requests = 0;
    }
    SolarTrendModel model(context);
    auto set_clock = [&] {
        model.setClock([&] { return now; });
        for (auto* src : {&model.kpSource(), &model.fluxSource(), &model.sunspotSource()}) src->setClock([&] { return now; });
    };
    set_clock();
    for (auto* src : {&model.kpSource(), &model.fluxSource(), &model.sunspotSource()}) QFile::remove(src->cachePath());
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(requests == 3, "three SWPC requests on start");
    check(model.state() == "live", "fresh data -> live");
    check(model.hasSunspots() && model.sunspotMonth() == "Sep 2026", "sunspot month text");
    check(model.hasFlux() && model.fluxLatest() == 123 && model.fluxHistory().size() == 30, "flux exposed");
    check(model.fluxCaption().contains("over 29 days") || model.fluxCaption().contains("over 30 days"), "flux caption");
    check(model.hasKp() && model.kpBars().size() == static_cast<int>(bars.size()), "kp bars exposed");
    check(model.kpBars().front().toMap().contains("level"), "bar carries colour level");

    // Outage after the freshness window keeps values but marks stale.
    fail_all = true;
    now = now.addDays(5);
    set_clock();
    for (auto* src : {&model.kpSource(), &model.fluxSource(), &model.sunspotSource()}) src->refreshNow();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale" && model.hasFlux(), "outage -> stale with last values kept");

    std::cout << "Solar trend: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
