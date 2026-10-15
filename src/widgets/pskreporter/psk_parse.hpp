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

// WSPR spots from wspr.live (ClickHouse, FORMAT JSON; columns time, tx_sign,
// tx_loc, rx_sign, rx_loc, frequency [Hz], snr, distance). Mode is "WSPR".
// Rows missing a call, a frequency or a time are skipped; a body that is not
// the expected JSON yields ok == false.
Feed parse_wspr(const QByteArray& body);
// Spots heard from `callsign` in the last hour (by DE), or empty when the call
// has characters outside [A-Z0-9/]. wspr.live asks for fair use: one query per
// 5 minutes, LIMIT 100.
QString wspr_query_url(const QString& callsign);
// Spots heard BY `callsign` in the last hour (of DE; same limits as above).
QString wspr_heard_url(const QString& callsign);
// Swaps sender and receiver so a "heard by DE" feed reads like a "hears DE"
// one: `receiver` becomes the station DE heard (reuses distance helpers).
std::vector<Report> heard_as_reports(const std::vector<Report>& heard);

// Query URL for "who heard this callsign", or an empty string when the call
// has characters outside [A-Z0-9/] (nothing is sent for it).
QString query_url(const QString& callsign);

} // namespace hamclock::psk
