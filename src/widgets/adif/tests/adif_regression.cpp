// ADIF parser and log model against a temp file. Offline.
#include "../adif_model.hpp"
#include "../adif_parse.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
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
    model.setEntityResolver([](const QString& c) { return c == "N0CALL" ? QString("ZZ") : QString("K"); });
    check(model.entityNeeded("N0CALL") && model.bandSlotNeeded("N0CALL", "20m"), "unworked entity needed");
    model.load("");
    check(!model.entityNeeded("N0CALL") && !model.bandSlotNeeded("N0CALL", "20m"), "unloaded log never claims needed");
    return failures ? 1 : 0;
}
