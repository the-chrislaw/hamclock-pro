#pragma once
// Parser for PSKReporter reception reports (retrieve.pskreporter.info/query,
// XML). Pure functions: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::psk {

struct Report {
    QString receiver, receiver_grid;  // who heard us (upper case)
    QString sender, sender_grid;
    double freq_khz = 0;
    QString band;                     // "20m", empty outside the amateur bands
    QString mode;
    int snr_db = 0;
    bool has_snr = false;
    QDateTime time;                   // UTC
};

struct Feed {
    bool ok = false;
    std::vector<Report> reports;      // newest first
};

// Rows missing a receiver, a usable frequency or a time are skipped. A body
// that is not a <receptionReports> document yields ok == false.
Feed parse_reports(const QByteArray& body);

// Query URL for "who heard this callsign", or an empty string when the call
// has characters outside [A-Z0-9/] (nothing is sent for it).
QString query_url(const QString& callsign);

} // namespace hamclock::psk
