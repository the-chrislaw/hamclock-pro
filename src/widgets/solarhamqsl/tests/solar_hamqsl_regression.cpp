// HamQSL widget: parser and model, offline.
// Fixture is a real https://www.hamqsl.com/solarxml.php response captured 2026-10-14.
#include "../solar_hamqsl_model.hpp"
#include "../solar_hamqsl_parse.hpp"
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
    QFile file(QDir(argv[1]).filePath("solarxml.xml"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray body = file.readAll();

    const auto r = solarhamqsl::parse_report(body);
    check(r.updated == QDateTime(QDate(2026, 10, 14), QTime(8, 11), QTimeZone::utc()), "updated time");
    check(r.hf.size() == 4 && r.hf[0].name == "80m-40m" && r.hf[0].day == "Poor" && r.hf[0].night == "Fair", "first HF band");
    check(r.hf[3].name == "12m-10m" && r.hf[3].day == "Fair" && r.hf[3].night == "Poor", "last HF band");
    check(r.vhf.size() == 5 && r.vhf[0].name == "vhf-aurora" && r.vhf[0].status == "Band Closed", "VHF items");

    check(solarhamqsl::parse_report("").hf.empty() && solarhamqsl::parse_report("<html>").hf.empty() &&
          solarhamqsl::parse_report("<solar><calculatedconditions><band name=\"a\" time=\"day\">Good</band>").hf.empty(),
          "garbage and truncated documents rejected");
    const auto half = solarhamqsl::parse_report(
        "<solar><solardata><updated> 1 Jan 2026 0000 GMT</updated><calculatedconditions>"
        "<band name=\"20m\" time=\"day\">Good</band></calculatedconditions></solardata></solar>");
    check(half.hf.empty(), "band with only a day value dropped");
    check(solarhamqsl::condition_rank("Good") == 3 && solarhamqsl::condition_rank("Fair") == 2 &&
          solarhamqsl::condition_rank("Poor") == 1 && solarhamqsl::condition_rank("?") == 0, "ranks");

    WidgetContext context;
    bool down = false;
    context.transport = [&](const QUrl&, FetchDone done) {
        FetchResult res;
        if (down) res.error = "network down"; else { res.http_status = 200; res.body = body; }
        QTimer::singleShot(0, [done, res] { done(res); });
    };
    auto settle = [&] { for (int i = 0; i < 20; ++i) app.processEvents(); };
    auto now = QDateTime(QDate(2026, 10, 14), QTime(9, 0), QTimeZone::utc());
    SolarHamqslModel model(context);
    model.setClock([&] { return now; });
    model.source().setClock([&] { return now; });
    QFile::remove(model.source().cachePath());
    check(!model.hasData() && model.state() == "loading" && model.bands().isEmpty(), "loading before data");
    model.start();
    settle();
    check(model.state() == "live" && model.hasData(), "live");
    check(model.bands().size() == 4 && model.vhf().size() == 5 && model.updated() == "08:11Z", "values");
    const auto first = model.bands().first().toMap();
    check(first["dayRank"].toInt() == 1 && first["nightRank"].toInt() == 2, "ranks in model");
    check(!model.vhf().first().toMap()["open"].toBool(), "closed band not open");

    now = now.addSecs(3 * 3600 + 1800);
    down = true;
    model.source().refreshNow();
    settle();
    check(model.state() == "stale" && model.hasData(), "outage -> stale, last values kept");

    now = now.addSecs(4 * 3600);  // report now 7 h old although the source answers again
    down = false;
    model.source().refreshNow();
    settle();
    check(!model.hasData() && model.bands().isEmpty(), "old report never shown as current");

    SolarHamqslModel offline(context);
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
