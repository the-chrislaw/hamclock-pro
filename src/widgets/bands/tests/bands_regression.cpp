// Band conditions widget: outlook model, A-index parser and model, offline.
// Fixtures are trimmed real NOAA SWPC responses captured 2026-10-13.
#include "../band_outlook.hpp"
#include "../bands_model.hpp"
#include "../../../net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
using hamclock::bands::Condition;
const hamclock::bands::BandOutlook& find(const std::vector<hamclock::bands::BandOutlook>& v, const char* name) {
    for (const auto& b : v) if (b.band == name) return b;
    static const hamclock::bands::BandOutlook none{};
    return none;
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    const QString dir = argv[1];

    // A index parser on the real product.
    const auto a = bands::latest_a_index(fixture(dir, "noaa-planetary-k-index.json"));
    check(a && a->value == 15, "latest A from real SWPC product");
    for (const char* bad : {"", "{}", "[", "[{\"time_tag\":\"2026-10-13T00:00:00\",\"a_running\":-1}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"a_running\":999}]",
                            "[{\"time_tag\":\"2026-10-13T00:00:00\",\"a_running\":\"5\"}]"})
        check(!bands::latest_a_index(bad), "malformed A rejected");

    // Outlook model.
    const auto mid = bands::hf_outlook(123, 3, 15);
    check(mid.size() == 8 && mid.front().band == "80m" && mid.back().band == "10m", "eight HF bands, 80m to 10m");
    check(find(mid, "20m").day == Condition::Good, "20m good by day at SFI 123");
    check(find(mid, "80m").day == Condition::Poor, "80m by day is absorbed");
    check(find(mid, "80m").night == Condition::Good, "80m good at night");
    check(find(mid, "40m").day == Condition::Fair, "40m by day capped at fair");
    check(find(mid, "10m").day == Condition::Poor && find(mid, "10m").night == Condition::Poor, "10m closed at SFI 123");
    const auto high = bands::hf_outlook(200, 2, 5);
    check(find(high, "10m").day == Condition::Good, "10m open by day at SFI 200");
    check(find(high, "10m").night == Condition::Poor, "10m closed at night even at SFI 200");
    const auto low = bands::hf_outlook(70, 1, 3);
    check(find(low, "15m").day == Condition::Poor && find(low, "20m").day != Condition::Poor, "low SFI closes 15m, keeps 20m");
    const auto storm = bands::hf_outlook(200, 5, 40);
    check(find(storm, "20m").day == Condition::Fair, "G1 storm degrades good to fair");
    const auto severe = bands::hf_outlook(200, 8, 120);
    check(find(severe, "20m").day == Condition::Poor && find(severe, "40m").night == Condition::Poor, "severe storm closes HF");
    const auto crazy = bands::hf_outlook(-5, 99, 1e9);
    check(crazy.size() == 8, "out-of-range inputs clamp, never crash");

    // Model end to end with a fixture transport.
    WidgetContext context;
    int requests = 0;
    bool fail_all = false;
    context.transport = [&](const QUrl& url, FetchDone done) {
        ++requests;
        FetchResult result;
        if (fail_all) result.error = "Host unreachable";
        else { result.http_status = 200; result.body = fixture(dir, QFileInfo(url.path()).fileName().toUtf8().constData()); }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    BandsModel model(context);
    const auto now = QDateTime(QDate(2026, 10, 13), QTime(16, 0), QTimeZone::utc());
    model.setClock([&] { return now; });
    for (auto* source : {&model.kpSource(), &model.fluxSource()}) {
        source->setClock([&] { return now; });
        QFile::remove(source->cachePath());
    }
    check(!model.hasOutlook() && model.state() == "loading", "no outlook before data");
    model.start();
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(requests == 2, "two SWPC requests on start");
    check(model.state() == "live", "fresh sources -> live");
    check(model.hasOutlook() && model.bands().size() == 8, "model outlook");
    check(model.inputsText() == "SFI 123 · Kp 3.0 · A 15", "inputs shown");
    check(model.period() == "unknown", "no station -> day/night unknown");

    fail_all = true;
    for (auto* source : {&model.kpSource(), &model.fluxSource()}) {
        source->setClock([&] { return now.addSecs(8 * 3600); });
        source->refreshNow();
    }
    model.setClock([&] { return now.addSecs(8 * 3600); });
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale", "outage -> stale, not live");
    check(model.hasOutlook(), "last outlook kept while stale");

    // Day/night at a site: noon vs midnight local at lon 0.
    check(bands::period_at(QDateTime(QDate(2026, 10, 13), QTime(12, 0), QTimeZone::utc()), 51.5, 0.0) == "day", "noon is day");
    check(bands::period_at(QDateTime(QDate(2026, 10, 13), QTime(0, 0), QTimeZone::utc()), 51.5, 0.0) == "night", "midnight is night");

    std::cout << "Bands: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
