#pragma once
// Parser for the HamQSL (N0NBH) solar-terrestrial XML feed (https://www.hamqsl.com/solarxml.php).
// Pure functions over the raw body. The HF/VHF condition words are N0NBH's own calculation.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::solarhamqsl {

struct HfBand {
    QString name;   // "80m-40m"
    QString day;    // "Poor" / "Fair" / "Good"
    QString night;
};

struct VhfItem {
    QString name;      // "E-Skip", "vhf-aurora"
    QString location;  // "europe", "north_america", "northern_hemi"
    QString status;    // "Band Closed" or an opening
};

struct Report {
    QDateTime updated;  // UTC, from <updated>
    std::vector<HfBand> hf;
    std::vector<VhfItem> vhf;
};

// Empty `hf` (and invalid `updated`) when the body is not the expected document.
Report parse_report(const QByteArray& body);

// 0 unknown, 1 poor, 2 fair, 3 good.
int condition_rank(const QString& word);

} // namespace hamclock::solarhamqsl
