#pragma once
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::adif {

struct Qso {
    QString call;  // uppercase
    QString band;  // lowercase, e.g. "20m"
    QString mode;  // uppercase
    QString grid;  // uppercase
    QDateTime time;  // UTC; invalid when QSO_DATE/TIME_ON are missing or malformed
};

// Parses ADIF text (.adi). Header is skipped; tag names are case-insensitive;
// field values are read by their declared length. Records without a CALL are
// dropped. Stops after max_records records.
std::vector<Qso> parse(const QString& text, int max_records = 100000);

// Calls compare without /P, /M, /QRP-style suffixes and EA8/ style prefixes.
QString base_call(const QString& call);

} // namespace hamclock::adif
