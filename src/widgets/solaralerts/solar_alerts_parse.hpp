#pragma once
// Parser for NOAA SWPC's space weather alerts, watches and warnings
// (products/alerts.json, public US-government data, no key). Pure functions
// over the raw body: no network, no clock besides the `now` passed in.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::solaralerts {

struct Alert {
    QString code;      // SWPC product id, e.g. "K05A"
    QString kind;      // Alert, Warning, Extended warning, Watch, Summary, Cancel
    QString headline;  // e.g. "Geomagnetic K-index of 5"
    QString scale;     // NOAA scale such as "G1", "S2", "R3"; empty if none stated
    QDateTime issued;  // UTC
    QDateTime until;   // UTC end of validity; invalid if the message has none
    bool cancelled = false;  // a cancellation notice, not an event
};

// All parseable messages, newest first. Messages without a headline or issue
// time are skipped.
std::vector<Alert> parse_alerts(const QByteArray& body);

// What is in force at `now`: newest message per product id; cancellations and
// the product they cancel are dropped; warnings only until their end time;
// alerts and summaries for 24 h; watches (no end time) for 48 h.
std::vector<Alert> active_alerts(const std::vector<Alert>& all, const QDateTime& now);

// Severity class for colouring: 0 info, 1 minor/moderate (scale 1-2 or a watch/warning), 2 strong+ (scale >= 3).
int alert_level(const Alert& alert);

} // namespace hamclock::solaralerts
