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
    const auto sys = zones.systemZone();
    CHECK(sys.isEmpty() || !zones.labelFor(sys).isEmpty());
    return failures ? 1 : 0;
}
