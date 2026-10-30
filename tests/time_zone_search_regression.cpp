#include <cmath>
#include "ui/time_zone_search.hpp"

#include <QCoreApplication>
#include <QVariantMap>
#include <cstdio>

using hamclock::TimeZoneSearch;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    TimeZoneSearch zones;
    const qint64 january = 1767225600;  // 2026-01-01T00:00Z, northern winter
    const qint64 july = 1782864000;     // 2026-07-01T00:00Z

    auto la = zones.search("los angeles", 10, january);
    CHECK(la.size() == 1);
    CHECK(la.value(0).toMap().value("id") == "America/Los_Angeles");
    CHECK(la.value(0).toMap().value("label").toString() == QString("Los Angeles (UTC") + QChar(0x2212) + "8)");
    CHECK(zones.labelFor("America/Los_Angeles", july).contains(QChar(0x2212) + QString("7)")));
    CHECK(zones.labelFor("Asia/Kolkata", january) == "Kolkata (UTC+5:30)");
    CHECK(zones.search("los_angeles", 10, january).size() == 1);   // underscore form matches too
    CHECK(zones.search("AMERICA los", 10, january).size() >= 1);  // every word, any case
    CHECK(zones.search("zzzz-no-such-place").isEmpty());
    CHECK(zones.search("", 5).size() == 5);                       // limit respected
    CHECK(zones.labelFor("Not/AZone").isEmpty());
    CHECK(zones.search("est5edt").isEmpty());                     // legacy aliases hidden
    CHECK(zones.suggestZone(40.7, -74.0) == "America/New_York");
    CHECK(zones.suggestZone(35.68, 139.7) == "Asia/Tokyo");
    CHECK(zones.suggestZone(51.0, 0.5) == "Europe/London");
    CHECK(zones.suggestZone(-33.9, 151.0) == "Australia/Sydney");
    CHECK(zones.suggestZone(0.0, -140.0).isEmpty());   // open Pacific: too far from any city
    CHECK(zones.suggestZone(95.0, 0.0).isEmpty());     // invalid latitude
    CHECK(zones.suggestZone(0.0, 200.0).isEmpty());    // invalid longitude
    const auto sys = zones.systemZone();
    CHECK(sys.isEmpty() || !zones.labelFor(sys).isEmpty());
    // The station's zone: a saved choice wins; otherwise it follows the station's
    // location, never the device's zone (Connecticut set up on a Mac in California).
    CHECK(zones.stationZone("Europe/Paris", 41.7292, -72.7083) == "Europe/Paris");
    CHECK(zones.stationZone("", 41.7292, -72.7083) == "America/New_York");
    CHECK(zones.stationZone("  ", 35.68, 139.69) == "Asia/Tokyo");
    // No usable location yet: the device's zone is the only hint left.
    CHECK(zones.stationZone("", std::nan(""), std::nan("")) == sys);
    CHECK(zones.stationZone("", 0.0, -140.0) == sys);
    return failures ? 1 : 0;
}
