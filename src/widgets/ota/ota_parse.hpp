#pragma once
// Parsers for On-The-Air activator spots: POTA (api.pota.app/spot/activator)
// and SOTA (api-db2.sota.org.uk/api/spots/<n>/all/all). Pure functions: no
// network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::ota {

struct Spot {
    QString program;     // "POTA" or "SOTA"
    QString call;        // activator, upper case
    QString reference;   // "US-1892" or "W4C/CM-009"
    QString name;        // park or summit name, may be empty
    double freq_khz = 0;
    QString mode;
    QDateTime time;      // UTC
    double latitude = 0, longitude = 0;
    bool has_position = false;
    QString comment;
};

struct Feed {
    bool ok = false;
    std::vector<Spot> spots;  // newest first, one per activator+reference
};

// Rows without a callsign, reference, usable frequency or time are skipped, as
// are spots whose comment says the activator has gone QRT. A body that is not
// a JSON array yields ok == false.
Feed parse_pota(const QByteArray& body);
Feed parse_sota(const QByteArray& body);  // SOTA frequencies are MHz on the wire

// Both feeds merged, newest first, duplicates (same program, call, reference)
// reduced to the newest spot.
std::vector<Spot> merge(const Feed& pota, const Feed& sota);

// Display filter. Empty band/mode and max_age_min <= 0 mean "no restriction".
struct Filter {
    QString band;          // "20m"; compared with the DX cluster band names
    QString mode;          // "CW", "SSB", ... case-insensitive
    int max_age_min = 0;
    QString program;       // "POTA" or "SOTA", empty for both
    // List order: "time" (newest first, default), "freq" (ascending) or "dist"
    // (nearest to DE first; spots without a position last). Unknown = "time".
    QString sort;
    bool has_de = false;
    double de_lat = 0, de_lon = 0;
};
std::vector<Spot> filter(const std::vector<Spot>& spots, const Filter& f, const QDateTime& now);

QString format_frequency(double khz);  // "14.064" MHz, "144.200"

} // namespace hamclock::ota
