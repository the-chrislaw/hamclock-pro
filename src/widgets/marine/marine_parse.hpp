#pragma once
// Parser for NWS active marine alerts (api.weather.gov/alerts/active, GeoJSON).
// US-government public domain, no key. Pure functions: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::marine {

struct Alert {
    QString id;
    QString event;     // "Gale Warning"
    QString area;      // NWS areaDesc
    QString office;    // issuing office, "NWS San Francisco CA"
    QDateTime sent;    // UTC
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
// 1 (advisory) .. 5 (hurricane force); 0 for events we do not track.
int event_rank(const QString& event);

// A body that is not an alert collection yields ok == false; an empty list is
// valid (nothing active). Rows without id, tracked event or expiry are skipped.
Feed parse(const QByteArray& body);

} // namespace hamclock::marine
