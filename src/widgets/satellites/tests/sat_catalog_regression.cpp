// Satellite alert catalog: the shipped JSON parses cleanly, every bird has a
// NORAD id, a known mode and status, sane frequencies, and the popular ones
// are present; malformed edits are skipped, not fatal. Offline.
#include "../sat_catalog.hpp"
#include "../../../domain/astro_satellite.hpp"
#include "../../../domain/astro_transmitters.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QSet>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const std::string& label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}

QByteArray read_file(const char* path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

void check_shipped(const hamclock::SatCatalog& cat) {
    using namespace hamclock;
    check(cat.entries.size() >= 15, "a useful number of satellites");
    check(!cat.updated.isEmpty(), "catalog says when it was last checked");
    QSet<int> seen;
    for (const CatalogEntry& e : cat.entries) {
        const std::string who = e.name.toStdString();
        check(e.norad > 0 && e.norad < 1000000, who + ": NORAD id");
        check(!seen.contains(e.norad), who + ": unique NORAD id");
        seen.insert(e.norad);
        check(!e.name.isEmpty() && !e.tle_name.isEmpty(), who + ": names");
        check(!e.transponders.isEmpty(), who + ": has a mode");
        check(e.status == "active" || e.status == "intermittent" || e.status == "inactive", who + ": status");
        for (const Transponder& t : e.transponders) {
            check(catalog_modes().contains(t.mode), who + ": known mode");
            check(t.downlink.valid() && t.downlink.lo_mhz > 20 && t.downlink.hi_mhz < 11000, who + ": downlink");
            check(!t.uplink.valid() || t.uplink.lo_mhz > 20, who + ": uplink");
            check(t.ctcss_hz == 0 || (t.ctcss_hz >= 60 && t.ctcss_hz <= 260), who + ": CTCSS");
            check(t.mode != "Linear" || t.downlink.hi_mhz > t.downlink.lo_mhz, who + ": linear passband");
        }
    }
    for (int norad : {25544, 27607, 43017, 43678, 44909, 7530, 22825, 61781})
        check(cat.find(norad) != nullptr, "popular bird present: " + std::to_string(norad));
    const CatalogEntry* iss = cat.find(25544);
    check(iss && iss->primaryMode() == "FM" && iss->transponders.size() >= 2 &&
          iss->transponders[1].mode == "APRS", "ISS voice repeater and APRS digipeater");
    check(iss && describe_transponder(iss->transponders[0]) == "FM 437.800 MHz down, 145.990 up, 67.0 Hz",
          "ISS repeater description");
    const CatalogEntry* rs44 = cat.find(44909);
    check(rs44 && format_freq_range(rs44->transponders[0].downlink) == "435.610-435.670", "linear passband text");
    const CatalogEntry* ao27 = cat.find(22825);
    check(ao27 && !ao27->workable(), "silent birds are not workable");
    check(cat.find(27607) && cat.find(27607)->workable(), "SO-50 workable");
    check(cat.find(1) == nullptr, "unknown id");
}

// Catalog TLE names match the CelesTrak fixture for the same NORAD id, so a renumbering is caught.
void check_against_tles(const hamclock::SatCatalog& cat, const QByteArray& tle_text) {
    using namespace hamclock;
    int matched = 0;
    for (const Tle& tle : parse_tle_set(tle_text.toStdString())) {
        const CatalogEntry* e = cat.find(tle_norad_id(tle));
        if (!e) continue;
        ++matched;
        check(QString::fromStdString(tle.name).trimmed() == e->tle_name,
              "TLE name for " + std::to_string(e->norad) + " is " + tle.name);
    }
    check(matched >= 12, "most catalog birds are in the CelesTrak amateur group");
}

void check_malformed() {
    using namespace hamclock;
    check(!parse_sat_catalog("not json").problems.isEmpty(), "bad JSON reported");
    check(parse_sat_catalog("not json").catalog.entries.isEmpty(), "bad JSON: no entries");
    const QByteArray mixed = R"json({"updated":"x","satellites":[
        {"norad":25544,"name":"ISS","tle_name":"ISS (ZARYA)","status":"active",
         "transponders":[{"mode":"FM","uplink_mhz":145.99,"downlink_mhz":437.8,"ctcss_hz":67}]},
        {"name":"NOID","tle_name":"X","status":"active","transponders":[{"mode":"FM","downlink_mhz":145.9}]},
        {"norad":5,"name":"BADMODE","tle_name":"X","status":"active","transponders":[{"mode":"CW","downlink_mhz":145.9}]},
        {"norad":6,"name":"NOMODE","tle_name":"X","status":"active","transponders":[]},
        {"norad":7,"name":"BADSTATUS","tle_name":"X","status":"maybe","transponders":[{"mode":"FM","downlink_mhz":145.9}]},
        {"norad":8,"name":"BADFREQ","tle_name":"X","status":"active","transponders":[{"mode":"FM","downlink_mhz":[146,145]}]},
        {"norad":25544,"name":"DUP","tle_name":"X","status":"active","transponders":[{"mode":"FM","downlink_mhz":145.9}]}
    ]})json";
    const CatalogParse p = parse_sat_catalog(mixed);
    check(p.catalog.entries.size() == 1 && p.catalog.entries[0].name == "ISS", "only the good entry kept");
    check(p.problems.size() == 6, "each bad entry reported");
    check(p.catalog.entries[0].transponders[0].ctcss_hz == 67.0, "tone parsed");
    check(format_freq_range({}) .isEmpty(), "empty range text");
    check(describe_transponder({"APRS", {145.825, 145.825}, {145.825, 145.825}, 0, {}}) == "APRS 145.825 MHz",
          "simplex digipeater description");
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 3) { std::cerr << "usage: test CATALOG.json AMATEUR.tle\n"; return 2; }
    const hamclock::CatalogParse shipped = hamclock::parse_sat_catalog(read_file(argv[1]));
    for (const QString& p : shipped.problems) std::cerr << "catalog problem: " << p.toStdString() << '\n';
    check(shipped.problems.isEmpty(), "shipped catalog has no problems");
    check_shipped(shipped.catalog);
    check_against_tles(shipped.catalog, read_file(argv[2]));
    check_malformed();
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
