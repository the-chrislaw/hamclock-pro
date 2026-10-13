#pragma once
// Parser for NWS active fire-weather alerts (api.weather.gov/alerts/active, GeoJSON).
// US-government public domain, no key. Pure functions: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::firewx {

struct Alert {
    QString id;
    QString event;     // "Red Flag Warning"
    QString area;      // NWS areaDesc
    QString office;    // issuing office, "NWS San Francisco CA"
    QDateTime sent;    // UTC
    QDateTime onset;   // UTC, invalid if none (watches start later)
    QDateTime expires; // UTC: "ends" when the alert gives it, else "expires"
    int rank = 0;      // severity rank from event(), higher is worse
};

struct Feed {
    bool ok = false;
    QDateTime updated; // feed build time, UTC
    std::vector<Alert> alerts; // worst first, then soonest expiry
};

// Comma-separated NWS event names requested from the API.
QString requested_events();
// 3 (watch) .. 4 (warning); 0 for events we do not track.
int event_rank(const QString& event);

// A body that is not an alert collection yields ok == false; an empty list is
// valid (nothing active). Rows without id, tracked event or expiry are skipped.
Feed parse(const QByteArray& body);

} // namespace hamclock::firewx
