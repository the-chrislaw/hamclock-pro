#pragma once
// Parser for the NHC active-storm index (www.nhc.noaa.gov/CurrentStorms.json):
// Atlantic and eastern/central Pacific tropical cyclones. US-government public
// domain, no key. Pure functions: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::storms {

struct Storm {
    QString id;              // "ep212026"
    QString name;            // "Trudy"
    QString classification;  // NHC code: TD, TS, HU, STD, STS, PTC, ...
    int wind_kt = 0;         // max sustained wind
    int pressure_mb = 0;     // 0 when not given
    double latitude = 0, longitude = 0;
    int heading_deg = -1;    // direction of movement, -1 unknown
    int speed_kt = -1;       // forward speed, -1 unknown
    QDateTime issued;        // advisory issuance, UTC
    QString url;             // public advisory page
};

struct Feed {
    bool ok = false;
    std::vector<Storm> storms;  // strongest first
};

// A body that is not an NHC index yields ok == false; an empty list is valid
// (no active storms). Rows without a name, wind or position are skipped.
Feed parse(const QByteArray& body);

// "Hurricane", "Tropical storm", ... from the NHC classification code.
QString classification_text(const QString& code);
// Saffir-Simpson category 1-5 for hurricanes (>= 64 kt), else 0.
int category(int wind_kt);
// Compass point for a heading in degrees ("NW"); empty for unknown.
QString compass(int degrees);

} // namespace hamclock::storms
