// Passes pane: 48 h schedule rows from real CelesTrak elements (fixture), filters, honest states. Offline.
#include "../passes_model.hpp"
#include "../../../domain/astro_transmitters.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimeZone>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    QFile file(QDir(argv[1]).filePath("celestrak-amateur.tle"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    // The pane schedules a handful of popular birds, not the whole group; five keeps the test quick.
    auto tles = parse_tle_set(file.readAll().toStdString());
    if (tles.size() > 5) tles.resize(5);
    check(tles.size() >= 5, "several element sets");
    if (tles.empty()) return 1;
    const double epoch = SatellitePropagator(tles.front()).epoch_unix();
    const QDateTime t0 = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(epoch) + 3600, QTimeZone::utc());

    check(pass_day_label(t0.toSecsSinceEpoch(), t0.toSecsSinceEpoch()) == "Today" &&
          pass_day_label(t0.toSecsSinceEpoch() + 86400, t0.toSecsSinceEpoch()) == "Tomorrow" &&
          pass_day_label(1792800000, 1792800000 - 3 * 86400).size() == 3, "day labels");

    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());
    WidgetContext context;
    context.settings = &settings;
    PassesModel model(context);
    model.setClock([&] { return t0; });

    model.recompute();
    check(model.state() == "unconfigured" && model.rows().isEmpty() && !model.emptyReason().isEmpty(), "no station: nothing invented");
    check(settings.saveStation("W1AW", 45.52, -122.68, 51.5, -0.12, "").isEmpty(), "station saved");
    model.recompute();
    check(model.state() == "loading" && model.rows().isEmpty(), "no elements yet: loading");

    std::set<int> amateur;
    amateur.insert(tle_norad_id(tles.front()));
    model.setElements(tles, amateur);
    const auto rows = model.rows();
    check(model.state() == "live" && rows.size() >= 3 && rows.size() <= 24, "live with a handful of passes");
    bool sorted = true, sane = true;
    qint64 prev = 0;
    for (const auto& r : rows) {
        const auto m = r.toMap();
        sorted = sorted && m["aos"].toLongLong() >= prev;
        prev = m["aos"].toLongLong();
        sane = sane && m["maxEl"].toInt() >= 10 && m["maxEl"].toInt() <= 90 && !m["name"].toString().isEmpty()
               && m["utc"].toString().endsWith("Z") && m["from"].toString().size() >= 1 && m["to"].toString().size() >= 1
               && m["duration"].toString().endsWith(" min");
    }
    check(sane && sorted, "rows well-formed");

    model.setFilter("amateur");
    const auto am = model.rows();
    check(model.filter() == "amateur" && !am.isEmpty() && am.size() <= rows.size(), "amateur filter narrows");
    const QString only = am.front().toMap()["name"].toString();
    bool same = true;
    for (const auto& r : am) same = same && r.toMap()["name"].toString() == only;
    check(same, "amateur filter keeps only the satellites with transmitters");
    check(QSettings(settings.optionsPath(), QSettings::IniFormat).value("passes/filter").toString() == "amateur", "filter saved");

    model.setFilter("visible");
    for (const auto& r : model.rows()) check(r.toMap()["visible"].toBool(), "visible filter keeps visible passes only");
    check(model.rows().size() <= rows.size(), "visible filter never adds passes");
    if (model.rows().isEmpty()) check(model.emptyReason().contains("visible"), "visible empty reason");

    model.setFilter("starred");
    check(model.rows().isEmpty() && model.emptyReason().contains("star"), "starred filter with no stars explains itself");
    const QString first = rows.front().toMap()["name"].toString();
    model.toggleStar(first);
    check(model.isStarred(first) && !model.rows().isEmpty(), "starring shows its passes");
    bool onlyStar = true;
    for (const auto& r : model.rows()) onlyStar = onlyStar && r.toMap()["name"].toString() == first && r.toMap()["starred"].toBool();
    check(onlyStar, "starred filter keeps only starred satellites");
    check(QSettings(settings.optionsPath(), QSettings::IniFormat).value("passes/starred").toStringList() == QStringList{first}, "stars saved");
    { PassesModel again(context); check(again.isStarred(first) && again.filter() == "starred", "stars and filter restored"); }
    model.toggleStar(first);
    check(!model.isStarred(first) && model.rows().isEmpty(), "unstar removes it");

    model.setFilter("bogus");
    check(model.filter() == "all" && model.rows().size() == rows.size(), "unknown filter -> all");

    // Elements far older than the cut-off are dropped, not scheduled.
    model.setClock([&] { return t0.addDays(60); });
    model.recompute();
    check(model.state() == "stale" && model.rows().isEmpty(), "old elements -> stale, no rows");

    std::cerr << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
