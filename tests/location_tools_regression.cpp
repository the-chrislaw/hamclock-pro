#include "ui/location_tools.hpp"

#include <QFile>
#include <cmath>
#include <cstdio>

using hamclock::LocationTools;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)
static bool near(double a, double b, double eps = 1e-4) { return std::fabs(a - b) < eps; }

int main(int argc, char** argv) {
    LocationTools t;
    CHECK(near(t.parseCoordinate("40.5", true), 40.5));
    CHECK(near(t.parseCoordinate("-74", false), -74));
    CHECK(near(t.parseCoordinate("40:44:55N", true), 40 + 44 / 60.0 + 55 / 3600.0));
    CHECK(near(t.parseCoordinate("73°59'11\" W", false), -(73 + 59 / 60.0 + 11 / 3600.0)));
    CHECK(near(t.parseCoordinate("S 33.9", true), -33.9));
    CHECK(near(t.parseCoordinate("151.2 e", false), 151.2));
    CHECK(std::isnan(t.parseCoordinate("40N", false)));   // wrong axis letter
    CHECK(std::isnan(t.parseCoordinate("91", true)));
    CHECK(std::isnan(t.parseCoordinate("181", false)));
    CHECK(std::isnan(t.parseCoordinate("40:75:00", true)));
    CHECK(std::isnan(t.parseCoordinate("-40 S", true)));
    CHECK(std::isnan(t.parseCoordinate("abc", true)));
    CHECK(std::isnan(t.parseCoordinate("", true)));

    const auto g = t.fromGrid("FN31pr");
    CHECK(near(g["latitude"].toDouble(), 41.7292, 1e-3) && near(g["longitude"].toDouble(), -72.7083, 1e-3));
    const auto g4 = t.fromGrid("JN58");
    CHECK(near(g4["latitude"].toDouble(), 48.5) && near(g4["longitude"].toDouble(), 11.0));
    CHECK(t.fromGrid("ZZ99").isEmpty() && t.fromGrid("FN3").isEmpty() && t.fromGrid("").isEmpty());
    CHECK(t.gridFor(41.7292, -72.7083) == "FN31pr");
    CHECK(t.gridFor(91, 0).isEmpty());
    CHECK(t.gridFor(90, 180) == "RR99xx");

    QFile f(argc > 1 ? argv[1] : "");
    CHECK(f.open(QIODevice::ReadOnly));
    const auto hits = t.parseSearchReply(f.readAll());
    CHECK(hits.size() == 1);
    if (hits.size() == 1) {
        const auto h = hits[0].toMap();
        CHECK(near(h["latitude"].toDouble(), 45.5189007) && near(h["longitude"].toDouble(), -122.6792966));
        CHECK(h["name"].toString().startsWith("Pioneer Courthouse Square"));
    }
    CHECK(t.parseSearchReply("not json").isEmpty());
    CHECK(t.parseSearchReply("[{\"lat\":\"x\",\"lon\":\"1\"},{\"lat\":\"95\",\"lon\":\"1\"}]").isEmpty());
    return failures ? 1 : 0;
}
