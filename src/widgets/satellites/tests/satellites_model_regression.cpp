// Satellites pane: TLE parsing into next-pass rows, offline.
// Fixture: six sets trimmed from a real CelesTrak amateur-group download captured 2026-10-13.
#include "../satellites_model.hpp"
#include "../../../net/polled_source.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

    check(pass_alert_text("AO-7", 1000, 1600, 1000 - 240) == "AO-7 rises in 4 min" &&
          pass_alert_text("AO-7", 1000, 1600, 1000 - 20) == "AO-7 rises in 1 min" &&
          pass_alert_text("AO-7", 1000, 1600, 1000 - 301).isEmpty() &&
          pass_alert_text("AO-7", 1000, 1600, 1200) == "AO-7 above horizon" &&
          pass_alert_text("AO-7", 1000, 1600, 1600).isEmpty(),
          "pass alert text");
    {
        SatPass a; a.aos = 1000; a.tca = 3600 + 14 * 3600 + 300; a.los = 1600; a.max_el_deg = 41.6;
        SatPass b = a; b.aos = 5000; b.los = 5600; b.max_el_deg = 77.6;
        SatPass late = a; late.aos = 100000; late.los = 100600;
        SatPass gone = a; gone.aos = -2000; gone.los = -1000;
        const QString s = pass_schedule_summary({{"FO-29", a}, {"AO-7", b}, {"AO-7", late}, {"ISS", gone}}, 0);
        check(s == "2 passes in 24 h, best AO-7 78° at 15:05Z", qPrintable("schedule: " + s));
        check(pass_schedule_summary({{"X", a}}, 0) .startsWith("1 pass in"), "singular");
        check(pass_schedule_summary({{"ISS", gone}}, 0).isEmpty() && pass_schedule_summary({}, 0).isEmpty(), "schedule empty");
    }
    {
        Transmitter off; off.active = false; off.downlink_low_hz = 1e6; off.mode = "FM";
        Transmitter fm; fm.active = true; fm.downlink_low_hz = 145.92e6; fm.mode = "FM";
        Transmitter tp; tp.active = true; tp.downlink_low_hz = 145.9e6; tp.downlink_high_hz = 146.0e6; tp.mode = "SSB";
        check(primary_downlink_text({off, fm}) == "145.9200 MHz FM", "primary skips inactive");
        check(primary_downlink_text({tp}) == "145.9000-146.0000 MHz SSB", "primary transponder range");
        check(primary_downlink_text({off}).isEmpty() && primary_downlink_text({}).isEmpty(), "primary none");
    }
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
    check(rows.size() >= 3 && rows.size() <= 12, "a handful of passes listed within 24 h");
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
        check(d["statusUrl"].toString().startsWith("https://db.satnogs.org/satellite/") && d["statusUrl"].toString().back().isDigit(),
              "detail carries a SatNOGS status link by NORAD id");
        const QVariantList more = d["more"].toList();
        check(more.size() <= 4, "at most four later passes");
        for (const auto& v : more)
            check(v.toMap()["aos"].toString().endsWith("Z") && v.toMap()["maxEl"].toInt() >= 10, "later passes carry a time and a peak above the threshold");
        const QVariantList tx = d["transmitters"].toList();
        bool known = name == "AO-7" || name == "ISS" || name == "SO-50" || name == "FO-29" || name == "AO-91";
        check(!known || !tx.isEmpty(), "transmitters joined by NORAD id for a satellite in the fixture");
        for (const auto& v : tx) check(v.toMap()["down"].toString().endsWith("MHz") || v.toMap()["up"].toString().endsWith("MHz"), "transmitter frequencies formatted");
        {
            // Show/hide track: toggles the first row's satellite in the saved choice.
            const bool was = d["tracked"].toBool();
            model.toggleTracked();
            check(model.detail()["tracked"].toBool() == !was, "toggleTracked flips the tracked state");
            QSettings ini(settings.optionsPath(), QSettings::IniFormat);
            check(ini.value("satellites/tracked").toStringList().contains(name, Qt::CaseInsensitive) == !was, "choice saved to options.ini");
            model.toggleTracked();
            check(model.detail()["tracked"].toBool() == was, "toggled back");
            ini.remove("satellites/tracked");
            ini.sync();
            model.recompute();
        }
        {
            // User TLE editor: invalid text is rejected without writing, valid text is saved and
            // loaded (a new NORAD id appears in the set), empty text removes the file.
            check(!model.saveUserTles("not a tle\nat all").isEmpty() && model.userTles().isEmpty(), "invalid user TLE rejected, nothing written");
            const QString good = "TEST SAT\n1 25544U 98067A   26286.11204827  .00005858  00000+0  11499-3 0  9991\n2 25544  51.6319  76.5194 0006724 256.4602 103.5638 15.48846742590035\n";
            check(model.saveUserTles(good).isEmpty() && model.userTles() == good, "valid user TLE saved");
            check(QFileInfo::exists(QFileInfo(settings.optionsPath()).absolutePath() + "/user-tles.txt"), "user-tles.txt beside options.ini");
            check(model.saveUserTles("  \n").isEmpty() && model.userTles().isEmpty(), "empty text clears");
        }
        {
            // Set as DX: DX moves to the sub-satellite point; DE and callsign are untouched.
            const double de_lat = settings.deLatitude(), de_lon = settings.deLongitude();
            const QString err = model.setAsDx();
            check(err.isEmpty(), "setAsDx saves");
            check(std::abs(settings.dxLatitude()) <= 90 && std::abs(settings.dxLatitude() - 51.5) > 0.01, "DX moved off the old target");
            check(settings.deLatitude() == de_lat && settings.deLongitude() == de_lon && settings.callsign() == "W1AW", "DE and callsign unchanged");
            const QString mutual = model.detail()["mutual"].toString();
            check(mutual.endsWith("to DX, max") == false && (mutual == "none in 48 h" || mutual.contains("(to DX, max")),
                  "detail carries the DE+DX co-visibility line once DX differs from DE");
        }
        check(toggle_tracked_names({}, "AO-7") == QStringList({"AO-7"}) &&
              toggle_tracked_names({"AO-7", "ISS"}, "iss") == QStringList({"AO-7"}) &&
              toggle_tracked_names({"AO-7", "ISS"}, "FO-29") == QStringList({"ISS", "FO-29"}), "toggle_tracked_names");
        model.selectRow(0);
        check(model.detail().isEmpty(), "second tap closes the detail");
        const QString first = rows.first().toMap()["name"].toString();
        check(!model.selectByName("NO-SUCH-SAT") && model.detail().isEmpty(), "unknown marker name opens nothing");
        check(model.selectByName(first.toLower()) && model.detail()["name"].toString() == first, "marker name opens its detail");
        check(model.selectByName(first) && !model.detail().isEmpty(), "selecting again keeps it open");
        model.selectRow(0);
        check(!model.setAsDx().isEmpty(), "setAsDx with no satellite open reports an error");
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
