#pragma once
// Parser for PSKReporter reception reports (retrieve.pskreporter.info/query,
// XML). Pure functions: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <utility>
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

// Reports per amateur band, most reports first (ties: lower band label order
// as first seen). Reports outside the amateur bands are skipped.
std::vector<std::pair<QString, int>> band_stats(const std::vector<Report>& reports);
// "20m 3 · 40m 1" for the first max_bands entries.
QString band_summary(const std::vector<std::pair<QString, int>>& stats, int max_bands);

// Reports per mode (upper case), most reports first; reports without a mode are
// skipped. Summary uses the same "FT8 5 · CW 2" form as band_summary.
std::vector<std::pair<QString, int>> mode_stats(const std::vector<Report>& reports);

// Query URL for "who heard this callsign", or an empty string when the call
// has characters outside [A-Z0-9/] (nothing is sent for it).
QString query_url(const QString& callsign);

} // namespace hamclock::psk
