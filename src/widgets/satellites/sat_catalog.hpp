#pragma once
// Curated list of amateur satellites people actually work (FM, linear, APRS,
// digital), shipped as amateur-satellites.json and matched to TLEs by NORAD id.
// The file carries a status per bird because satellites go quiet; edit the
// JSON, not this code, when AMSAT reports a change.
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace hamclock {

// A frequency or a linear passband in MHz; lo == hi for a channel, 0 = none.
struct FreqRange {
    double lo_mhz = 0;
    double hi_mhz = 0;
    bool valid() const { return lo_mhz > 0 && hi_mhz >= lo_mhz; }
};

struct Transponder {
    QString mode;      // "FM", "Linear", "APRS" or "Digital"
    FreqRange uplink;
    FreqRange downlink;
    double ctcss_hz = 0; // 0 = no tone
    QString label;
};

struct CatalogEntry {
    int norad = 0;
    QString name;      // short display name, "SO-50"
    QString tle_name;  // as CelesTrak spells it, for reference only
    QString status;    // "active", "intermittent" or "inactive"
    QString note;
    QVector<Transponder> transponders; // never empty for a parsed entry
    // Active and intermittent birds are "workable": what "All workable" alerts on.
    bool workable() const { return status != QLatin1String("inactive"); }
    QString primaryMode() const { return transponders.isEmpty() ? QString() : transponders.front().mode; }
};

struct SatCatalog {
    QString updated;
    QVector<CatalogEntry> entries;
    const CatalogEntry* find(int norad) const;
};

struct CatalogParse {
    SatCatalog catalog;
    QStringList problems; // one line per skipped entry or file-level error; empty when clean
};

// The modes a transponder may have, in the order the Settings groups show them.
const QStringList& catalog_modes();

// Parses the JSON catalog. Malformed entries are skipped (and listed in
// `problems`) so one bad edit never silences every alert; duplicate NORAD ids keep the first.
CatalogParse parse_sat_catalog(const QByteArray& json);

// The catalog bundled with the app (qrc:/satalerts/amateur-satellites.json), parsed
// once; empty where the resource is not linked in (unit tests).
const SatCatalog& shipped_sat_catalog();

// "437.800" or "435.610-435.670" (MHz, 3 decimals); empty when invalid.
QString format_freq_range(const FreqRange& range);
// "FM 437.800 MHz down, 145.990 up, 67.0 Hz" (tone and uplink only when present).
QString describe_transponder(const Transponder& t);

} // namespace hamclock
