// Satellites pane: TLE parsing into next-pass rows, offline.
// Fixture: six sets trimmed from a real CelesTrak amateur-group download captured 2026-10-13.
#include "../satellites_model.hpp"
#include "../../../net/polled_source.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QTimer>
#include <QVariantMap>
#include <algorithm>
#include <cmath>
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
    QFile file(QDir(argv[1]).filePath("gp.php"));
    check(file.open(QIODevice::ReadOnly), "fixture present");
    const QByteArray body = file.readAll();
    QFile txfile(QDir(argv[1]).filePath("transmitters.json"));
    check(txfile.open(QIODevice::ReadOnly), "transmitter fixture present");
    const QByteArray txbody = txfile.readAll();

    check(satellite_short_name("OSCAR 7 (AO-7)   ") == "AO-7" && satellite_short_name("ISS (ZARYA)") == "ISS" &&
          satellite_short_name("RS-44 & BREEZE-KM R/B") == "RS-44" && satellite_short_name("  FOO  ") == "FOO",
          "short names");

    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());
    WidgetContext context;
    context.settings = &settings;
    bool down = false;
    context.transport = [&](const QUrl& url, FetchDone done) {
        FetchResult r;
        if (down) r.error = "network down"; else { r.http_status = 200; r.body = url.host() == "db.satnogs.org" ? txbody : body; }
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    auto settle = [&] { for (int i = 0; i < 20; ++i) app.processEvents(); };
    const QDateTime t0(QDate(2026, 10, 14), QTime(12, 0), QTimeZone::utc());

    SatellitesModel model(context);
    model.setClock([&] { return t0; });
    model.source().setClock([&] { return t0; });
    model.transmitterSource().setClock([&] { return t0; });
    QFile::remove(model.source().cachePath());
    QFile::remove(model.transmitterSource().cachePath());
    // Station not configured yet: data loads but no passes are invented.
    model.start();
    settle();
    check(model.hasData() && model.rows().isEmpty(), "no station -> no rows");

    check(settings.saveStation("W1AW", 45.52, -122.68, 51.5, -0.12, "").isEmpty(), "station saved");
    settle();
    const auto rows = model.rows();
    check(model.state() == "live" && model.hasData(), "live");
    check(rows.size() >= 3 && rows.size() <= 5, "a handful of passes listed within 24 h");
    bool sane = true;
    for (const auto& r : rows) {
        const auto m = r.toMap();
        const int el = m["maxEl"].toInt();
        sane = sane && el >= 10 && el <= 90 && !m["name"].toString().isEmpty() && !m["when"].toString().isEmpty();
    }
    check(sane, "rows have name, time and 10-90 degree peak elevation");

    {
        const auto sky = model.skyTrack();
        bool ok = sky.size() >= 2;
        double peak = 0;
        for (const auto& v : sky) {
            const auto m = v.toMap();
            ok = ok && m["az"].toDouble() >= 0 && m["az"].toDouble() <= 360 && m["el"].toDouble() >= -1 && m["el"].toDouble() <= 90;
            peak = std::max(peak, m["el"].toDouble());
        }
        check(ok && sky.size() <= 24, "sky track: samples in range");
        check(!sky.isEmpty() && std::abs(sky.first().toMap()["el"].toDouble() - 10) < 1.5 &&
              std::abs(sky.last().toMap()["el"].toDouble() - 10) < 1.5, "sky track starts and ends at the 10 degree pass threshold");
        check(peak <= rows.first().toMap()["maxEl"].toInt() + 1.5 && peak > 5, "sky track peak is within the first pass maximum");
    }
    {
        // Detail box: tap a row, see az/el, the pass and the transmitters of that satellite.
        check(model.detail().isEmpty(), "no detail before a tap");
        model.selectRow(0);
        const QVariantMap d = model.detail();
        const QString name = rows.first().toMap()["name"].toString();
        check(d["name"].toString() == name && d["maxEl"].toInt() == rows.first().toMap()["maxEl"].toInt(), "detail matches the tapped row");
        check(d["az"].toInt() >= 0 && d["az"].toInt() <= 360 && !d["compass"].toString().isEmpty() &&
              d["rangeKm"].toInt() > 300 && d["rangeKm"].toInt() < 20000 && !d["aosAz"].toString().isEmpty(), "detail look angle and pass azimuths sane");
        const QVariantList more = d["more"].toList();
        check(more.size() <= 4, "at most four later passes");
        for (const auto& v : more)
            check(v.toMap()["aos"].toString().endsWith("Z") && v.toMap()["maxEl"].toInt() >= 10, "later passes carry a time and a peak above the threshold");
        const QVariantList tx = d["transmitters"].toList();
        bool known = name == "AO-7" || name == "ISS" || name == "SO-50" || name == "FO-29" || name == "AO-91";
        check(!known || !tx.isEmpty(), "transmitters joined by NORAD id for a satellite in the fixture");
        for (const auto& v : tx) check(v.toMap()["down"].toString().endsWith("MHz") || v.toMap()["up"].toString().endsWith("MHz"), "transmitter frequencies formatted");
        model.selectRow(0);
        check(model.detail().isEmpty(), "second tap closes the detail");
        check(format_mhz(145800000) == "145.8000 MHz" && compass_point(0) == "N" && compass_point(359) == "N" &&
              compass_point(90) == "E" && compass_point(225) == "SW", "format helpers");
    }
    {
        const OverlayLayer* layer = nullptr;
        for (const auto& l : MapOverlays::instance().layers()) if (l.id == "satellites") layer = &l;
        check(layer != nullptr && layer->markers.size() >= 21, "satellites layer published with tracks");
        int labelled = 0; bool ok = layer != nullptr;
        if (layer) for (const auto& m : layer->markers) {
            if (!m.call.isEmpty()) ++labelled;
            ok = ok && m.where.latitude >= -90 && m.where.latitude <= 90 && m.where.longitude >= -180 && m.where.longitude <= 180;
        }
        check(labelled == 2 && ok, "default: two labelled sub-points (soonest passes), coordinates in range");
    }
    {
        // User choice from options.ini wins, capped at two; unknown names are ignored.
        QSettings ini(settings.optionsPath(), QSettings::IniFormat);
        ini.setValue("satellites/tracked", QStringList{"nonesuch", "ao-7", "ISS", "FO-29"});
        ini.sync();
        model.recompute();
        QStringList names;
        for (const auto& l : MapOverlays::instance().layers())
            if (l.id == "satellites") for (const auto& m : l.markers) if (!m.call.isEmpty()) names << m.call;
        names.sort();
        check(names == QStringList({"AO-7", "ISS"}), "chosen satellites only, at most two");
        ini.remove("satellites/tracked");
        ini.sync();
        model.recompute();
    }

    // Elements older than 30 days are dropped, not propagated as if live.
    {
        QDateTime later = t0.addDays(60);
        model.setClock([&] { return later; });
        model.recompute();
        bool layer = false;
        for (const auto& l : MapOverlays::instance().layers()) if (l.id == "satellites") layer = true;
        check(model.hasData() && model.allStale() && model.rows().isEmpty() && !layer, "60-day-old elements -> stale, no rows or tracks");
        model.setClock([&] { return t0; });
        model.recompute();
        check(!model.allStale() && !model.rows().isEmpty(), "fresh again");
    }

    down = true;
    model.source().refreshNow();
    settle();
    check(model.hasData() && !model.rows().isEmpty(), "outage keeps last elements");

    SatellitesModel bad(context);
    bad.setClock([&] { return t0; });
    bad.source().setClock([&] { return t0; });
    QFile::remove(bad.source().cachePath());
    bad.start();
    settle();
    check(!bad.hasData() && bad.rows().isEmpty() && bad.state() == "offline", "no data and down -> offline, no rows");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
