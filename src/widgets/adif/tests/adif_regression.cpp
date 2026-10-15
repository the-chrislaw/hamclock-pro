// ADIF parser and log model against a temp file. Offline.
#include "../adif_model.hpp"
#include "../adif_overlay.hpp"
#include "../adif_parse.hpp"
#include "../../../map/overlay.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
const char* kLog =
    "ADIF export from some logger\n<ADIF_VER:5>3.1.4\n<PROGRAMID:3>foo <EOH>\n"
    "<call:5>W1AW  <BAND:3>20m <MODE:3>SSB <QSO_DATE:8>20260101 <TIME_ON:4>1230 <GRIDSQUARE:6>fn31pr <eor>\n"
    "<CALL:10:S>EA8/DL1ABC<BAND:3>40M<MODE:2>CW<QSO_DATE:8>20260301<TIME_ON:6>081500<EOR>\n"
    "<BAND:3>20m<EOR>\n"                              // no call: dropped
    "<CALL:4>K1XX<COMMENT:99>truncated<EOR>\n"        // bad length: field skipped, record kept
    "<CALL:5>VK9XX<QSO_DATE:8>notadate<EOR>\n";
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    const auto q = adif::parse(kLog);
    check(q.size() == 4, "four records with a call");
    check(q.size() > 0 && q[0].call == "W1AW" && q[0].band == "20m" && q[0].mode == "SSB" && q[0].grid == "FN31PR", "fields parsed, case folded");
    check(q.size() > 0 && q[0].time == QDateTime(QDate(2026, 1, 1), QTime(12, 30), Qt::UTC), "HHMM time");
    check(q.size() > 1 && q[1].call == "EA8/DL1ABC" && q[1].band == "40m" && q[1].time.time() == QTime(8, 15), "type indicator and HHMMSS");
    check(q.size() > 3 && !q[3].time.isValid(), "bad date gives invalid time");
    check(adif::parse("").empty() && adif::parse("<").empty() && adif::parse("<CALL:5>W1").empty(), "garbage safe");
    check(adif::parse(kLog, 2).size() == 2, "record cap");
    check(adif::base_call("EA8/DL1ABC") == "DL1ABC" && adif::base_call("w1aw/p") == "W1AW" && adif::base_call("W1AW") == "W1AW", "base_call");

    QTemporaryDir dir;
    QFile f(dir.path() + "/log.adi");
    f.open(QIODevice::WriteOnly);
    f.write(kLog);
    f.close();
    WidgetContext ctx;
    AdifModel model(ctx);
    model.load("");
    check(model.state() == "unconfigured" && !model.hasData(), "no path -> unconfigured");
    model.load(dir.path() + "/missing.adi");
    check(model.state() == "error", "missing file -> error");
    model.load(f.fileName());
    check(model.state() == "live" && model.count() == 4, "loads file");
    {
        const auto& layers = MapOverlays::instance().layers();
        check(layers.size() == 1 && layers[0].id == "adif" && layers[0].markers.size() == 1, "only QSOs with a grid are drawn");
        const auto& m = layers[0].markers[0];
        // FN31pr: SW corner 41.7083, -72.75; centre adds half a 5 x 2.5 arc-minute subsquare
        check(m.call == "W1AW" && std::abs(m.where.latitude - 41.7292) < 0.01 && std::abs(m.where.longitude - (-72.7083)) < 0.01, "marker at grid cell centre");
    }
    model.setMapVisible(false);
    check(MapOverlays::instance().layers().empty(), "map off removes the layer");
    model.setMapVisible(true);
    check(MapOverlays::instance().layers().size() == 1, "map on restores it");
    {
        std::vector<adif::Qso> qs;
        auto mk = [](const char* c, const char* g) { adif::Qso q; q.call = c; q.grid = g; return q; };
        qs = {mk("K1AA", "FN31"), mk("K1AA/P", "JO62"), mk("K2BB", "ZZ99"), mk("K3CC", "FN3"), mk("K4DD", "JO62pq12")};
        const auto l = adif::overlay_layer(qs, 10);
        check(l.markers.size() == 2 && l.markers[0].where.latitude == 41.5 && l.markers[0].where.longitude == -73.0,
              "newest per call, invalid or odd-length grids skipped, 4-char centre");
        check(adif::overlay_layer(qs, 1).markers.size() == 1, "marker cap");
    }
    check(model.recent().size() == 4 && model.recent()[0].toMap()["call"] == "EA8/DL1ABC", "newest first");
    check(model.worked("dl1abc") && model.worked("W1AW/P") && !model.worked("N0CALL"), "worked-before ignores portable parts");

    // NADXCC / NABand with a synthetic resolver (EA8 -> EA8, W/K -> K, DL -> DL).
    model.setEntityResolver([](const QString& c) {
        const QString b = adif::base_call(c);
        return c.startsWith("EA8/") ? QString("EA8") : b.startsWith("DL") ? QString("DL") : b.startsWith("W") || b.startsWith("K") ? QString("K") : QString();
    });
    check(model.state() == "live", "reloaded after resolver change");
    check(!model.entityNeeded("W9XYZ") && !model.entityNeeded("VK9XX"), "worked entity / unknown entity not needed");
    check(model.entityNeeded("DL5ABC") && !model.entityNeeded("EA8/DL1ABC"), "operating prefix decides the entity");
    check(!model.bandSlotNeeded("K2AAA", "20m") && !model.bandSlotNeeded("K2AAA", "20M") && model.bandSlotNeeded("K2AAA", "15m"), "band slot needed only when entity+band unworked");
    check(!model.bandSlotNeeded("EA8/XX1X", "40m") && model.bandSlotNeeded("EA8/XX1X", "20m") && !model.bandSlotNeeded("VK9XX", "20m"), "slots per entity; unknown entity never needed");
    check(adif::prefix_of("K1ABC") == "K1" && adif::prefix_of("9A1A") == "9A1" && adif::prefix_of("EA8/DL1ABC") == "EA8" &&
          adif::prefix_of("DL5ABC/P") == "DL5" && adif::prefix_of("3DA0XX") == "3DA0" && adif::prefix_of("NODIGIT").isEmpty(),
          "prefix_of");
    check(model.prefixNeeded("DL5ABC") && !model.prefixNeeded("EA8/XX1X") && !model.prefixNeeded("W1ZZZ") && model.prefixNeeded("W2ZZZ"),
          "prefix needed only when no logged call shares it");
    check(!model.gridNeeded("FN31") && !model.gridNeeded("fn31xx") && model.gridNeeded("JO62") && model.gridNeeded("IO91wm") &&
          !model.gridNeeded("") && !model.gridNeeded("ZZ99") && !model.gridNeeded("FN3"),
          "grid needed only for valid 4-char locators absent from the log");
    model.setEntityResolver([](const QString& c) { return c == "N0CALL" ? QString("ZZ") : QString("K"); });
    check(model.entityNeeded("N0CALL") && model.bandSlotNeeded("N0CALL", "20m"), "unworked entity needed");
    model.load("");
    check(MapOverlays::instance().layers().empty(), "no log leaves nothing on the map");
    check(!model.entityNeeded("N0CALL") && !model.bandSlotNeeded("N0CALL", "20m"), "unloaded log never claims needed");
    return failures ? 1 : 0;
}
