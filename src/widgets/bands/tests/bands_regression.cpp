// Band conditions widget: outlook model, A-index parser and model, offline.
// Fixtures are trimmed real NOAA SWPC responses captured 2026-10-13.
#include "../band_outlook.hpp"
#include "../bands_model.hpp"
#include "../../../map/styles.hpp"
#include "../../../net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <vector>
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
QByteArray kp_row(const QDateTime& time, double kp = 3.0) {
    return QString(R"([{"time_tag":"%1","Kp":%2,"a_running":15,"station_count":8}])")
        .arg(time.toString("yyyy-MM-ddTHH:mm:ss")).arg(kp).toUtf8();
}
QByteArray flux_row(const QDateTime& time) {
    return QString(R"([{"flux":123,"time_tag":"%1"}])").arg(time.toString("yyyy-MM-ddTHH:mm:ss")).toUtf8();
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
    QByteArray kp_body, flux_body;  // when set, the feed answers with this instead of the fixture
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
        else {
            const auto name = QFileInfo(url.path()).fileName();
            const auto& body = name == "10cm-flux.json" ? flux_body : kp_body;
            result.http_status = 200;
            result.body = !body.isEmpty() ? body : fixture(dir, name.toUtf8().constData());
        }
        QTimer::singleShot(0, [done, result] { done(result); });
    };
    const auto now = QDateTime(QDate(2026, 10, 13), QTime(16, 0), QTimeZone::utc());
    // Startup race: solar flux arrives first, Kp/A still in flight -> loading,
    // never "returned no usable data".
    {
        hold = {"noaa-planetary-k-index.json"};
        BandsModel early(context);
        early.setClock([&] { return now; });
        for (auto* source : {&early.kpSource(), &early.fluxSource()}) {
            source->setClock([&] { return now; });
            QFile::remove(source->cachePath());
        }
        early.start();
        for (int i = 0; i < 20; ++i) app.processEvents();
        check(early.state() == "loading" && !early.statusText().contains("no usable"),
              "one index still in flight -> loading");
        hold.clear();
        auto pending = std::move(held);
        for (auto& [done, name] : pending) {
            FetchResult r;
            r.http_status = 200;
            r.body = fixture(dir, name.toUtf8().constData());
            done(r);
        }
        for (int i = 0; i < 20; ++i) app.processEvents();
        check(early.state() == "live", "both arrived -> live");
        requests = 0;
    }
    BandsModel model(context);
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
    {
        bool found = false;
        for (const auto& st : MapStyles::instance().styles()) found = found || (st.id == "muf" && !st.image.isNull());
        check(found, "fresh indices publish the MUF map style");
    }

    fail_all = true;
    for (auto* source : {&model.kpSource(), &model.fluxSource()}) {
        source->setClock([&] { return now.addSecs(8 * 3600); });
        source->refreshNow();
    }
    model.setClock([&] { return now.addSecs(8 * 3600); });
    for (int i = 0; i < 20; ++i) app.processEvents();
    check(model.state() == "stale", "outage -> stale, not live");
    check(model.hasOutlook(), "last outlook kept while stale");
    {
        bool found = false;
        for (const auto& st : MapStyles::instance().styles()) found = found || st.id == "muf";
        check(!found, "stale indices remove the MUF map style");
    }

    // The age of the values themselves, with both feeds answering on time.
    fail_all = false;
    {
        struct Seen { QString state, status; bool has_outlook, has_muf; QDateTime updated; };
        const auto seen = [&](double kp_hours, double sfi_hours) {
            kp_body = kp_row(now.addSecs(qint64(-kp_hours * 3600)));
            flux_body = flux_row(now.addSecs(qint64(-sfi_hours * 3600)));
            BandsModel aged(context);
            aged.setClock([&] { return now; });
            for (auto* source : {&aged.kpSource(), &aged.fluxSource()}) {
                source->setClock([&] { return now; });
                QFile::remove(source->cachePath());
            }
            aged.start();
            for (int i = 0; i < 20; ++i) app.processEvents();
            bool muf = false;
            for (const auto& st : MapStyles::instance().styles()) muf = muf || st.id == "muf";
            return Seen{aged.state(), aged.statusText(), aged.hasOutlook(), muf, aged.updatedAt()};
        };
        // Healthy feeds. The Kp tag is the start of its 3 h interval and the row follows the
        // interval's end, so 6 h and a bit is normal; SFI is daily, with a weekend possible.
        check(seen(3.8, 4.8).state == "live", "Kp and SFI as the real feeds looked -> live");
        check(seen(8, 24).state == "live", "Kp row running late, SFI a day old -> live");
        check(seen(3, 73).state == "live", "SFI from Friday, on Monday -> live");
        // One input of two is old: named, with the outlook kept.
        const auto old_kp = seen(10, 20);
        check(old_kp.state == "stale", "old Kp, feed still answering -> stale");
        check(old_kp.status.contains("old Kp") && !old_kp.status.contains("SFI"), "note names only the old Kp");
        check(old_kp.has_outlook, "old Kp keeps the outlook on screen");
        check(!old_kp.has_muf, "old Kp removes the MUF map style");
        check(old_kp.updated == now.addSecs(-10 * 3600), "age shown is the old reading's");
        const auto old_sfi = seen(3, 85);
        check(old_sfi.state == "stale", "SFI past a weekend, feed still answering -> stale");
        check(old_sfi.status.contains("old SFI") && !old_sfi.status.contains("Kp"), "note names only the old SFI");
        check(old_sfi.has_outlook, "old SFI keeps the outlook on screen");
        const auto both = seen(10, 85);
        check(both.state == "stale" && both.status.contains("SFI") && both.status.contains("Kp"), "both old -> both named");
        check(both.has_outlook, "both old keeps the outlook on screen");
        for (const auto& note : {old_kp.status, old_sfi.status, both.status})
            check(note.size() <= 28, "stale note fits the 800x480 footer");

        // The feed goes on answering with the same rows while they age: no new body is needed.
        kp_body.clear();
        flux_body.clear();
        BandsModel frozen(context);
        QDateTime clock = now;
        frozen.setClock([&] { return clock; });
        for (auto* source : {&frozen.kpSource(), &frozen.fluxSource()}) {
            source->setClock([&] { return clock; });
            QFile::remove(source->cachePath());
        }
        frozen.start();
        for (int i = 0; i < 20; ++i) app.processEvents();
        check(frozen.state() == "live", "fixture rows at their capture time -> live");
        clock = now.addSecs(6 * 3600);
        for (auto* source : {&frozen.kpSource(), &frozen.fluxSource()}) source->refreshNow();
        for (int i = 0; i < 20; ++i) app.processEvents();
        check(frozen.state() == "stale" && frozen.statusText().contains("old Kp"), "same rows, hours later -> stale");
    }

    // Day/night at a site: noon vs midnight local at lon 0.
    check(bands::period_at(QDateTime(QDate(2026, 10, 13), QTime(12, 0), QTimeZone::utc()), 51.5, 0.0) == "day", "noon is day");
    check(bands::period_at(QDateTime(QDate(2026, 10, 13), QTime(0, 0), QTimeZone::utc()), 51.5, 0.0) == "night", "midnight is night");

    std::cout << "Bands: " << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
