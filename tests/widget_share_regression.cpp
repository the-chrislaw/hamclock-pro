// Station hand-off to the iPhone widgets: validation and the App Group key contract
// (apple/widgets/Shared/AppGroup.swift reads the same keys).
#include "../src/platform/apple_widgetshare.hpp"
#include <QCoreApplication>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
hamclock::StationSettings station(double lat, double lon, const char* call) {
    hamclock::StationSettings s;
    s.de = hamclock::Location{lat, lon};
    s.callsign = call;
    return s;
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    // Grid-square centre (FN31), not a real address.
    const auto de = station(41.5, -73.0, " n0call ");

    auto s = makeWidgetShare(de, true, {"ISS", " SO-50 "});
    check(s.configured && s.callsign == "N0CALL", "configured station, callsign trimmed and upper-cased");
    check(s.latitude == 41.5 && s.longitude == -73.0, "position kept");
    check(s.tracked == QStringList({"ISS", "SO-50"}), "tracked names trimmed");

    s = makeWidgetShare(de, false, {"ISS"});
    check(!s.configured && s.callsign.isEmpty() && s.latitude == 0 && s.tracked == QStringList({"ISS"}),
          "unconfigured station shares no position but keeps tracked names");
    check(!makeWidgetShare(station(91, 0, "N0CALL"), true, {}).configured, "latitude out of range");
    check(!makeWidgetShare(station(0, 181, "N0CALL"), true, {}).configured, "longitude out of range");
    check(!makeWidgetShare(station(std::numeric_limits<double>::quiet_NaN(), 0, "N0CALL"), true, {}).configured, "NaN rejected");

    s = makeWidgetShare(de, true, {"", "iss", "ISS", QString(40, 'X'), "AO-91", "RS-44"});
    check(s.tracked == QStringList({"iss", "AO-91"}), "empty, duplicate and overlong names dropped, at most two");

    const auto f = widgetShareFields(makeWidgetShare(de, true, {"ISS"}), 1793000000);
    check(f.value("share.version").toInt() == kWidgetShareVersion && f.value("share.updated").toLongLong() == 1793000000,
          "version and timestamp");
    check(f.value("station.configured").toBool() && f.value("station.callsign").toString() == "N0CALL", "station keys");
    check(f.value("station.lat").toDouble() == 41.5 && f.value("station.lon").toDouble() == -73.0, "position keys");
    check(f.value("satellites.tracked").toStringList() == QStringList({"ISS"}), "tracked key");
    check(f.size() == 7, "no extra keys");

    const auto off = widgetShareFields(makeWidgetShare(de, false, {}), 1);
    check(!off.value("station.configured").toBool() && off.value("station.callsign").toString().isEmpty() &&
              off.value("station.lat").toDouble() == 0, "unconfigured fields are cleared");

    check(std::string(kWidgetAppGroup) == "group.com.chrislaw.hamclockpro", "App Group id");
    check(!nativeShareForWidgets(s), "no native write off iOS");
    return failures ? 1 : 0;
}
