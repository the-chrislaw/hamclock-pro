#pragma once
// Parsers for NOAA SWPC real-time solar wind products (json/rtsw/*_1m.json,
// public US-government data, no key). Pure functions: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::solarwind {

struct Point {
    QDateTime time;  // UTC, one-minute cadence
    double value = 0;
};

// The feeds interleave the active spacecraft (e.g. DSCOVR/SOLAR1) with backup
// ones; only rows with "active": true are used. Rows whose field is null,
// non-numeric or a -9999 style fill value are skipped. Returned oldest first,
// at most one point per minute.
std::vector<Point> series(const QByteArray& body, const char* field);

// Field names in the products.
inline constexpr char kSpeed[] = "proton_speed";    // km/s  (rtsw_wind_1m)
inline constexpr char kDensity[] = "proton_density"; // p/cm^3 (rtsw_wind_1m)
inline constexpr char kBt[] = "bt";                  // nT    (rtsw_mag_1m)
inline constexpr char kBz[] = "bz_gsm";              // nT, GSM (rtsw_mag_1m)

// Short plain-language assessments used for card colouring and captions.
QString speed_description(double km_s);  // Slow / Moderate / Fast / Very fast
QString bz_description(double nt);       // Northward / Weakly south / Southward / Strongly south

} // namespace hamclock::solarwind
