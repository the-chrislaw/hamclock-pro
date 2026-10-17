#pragma once
#include <QHash>
#include <QByteArray>
// Parser for "DX de" spot lines as sent by DXSpider, AR-Cluster and CC-Cluster.
#include <QDateTime>
#include <QString>
#include <optional>

namespace hamclock::dxcluster {

struct DxSpot {
    QString spotter;      // without the cluster SSID suffix colon
    QString dx;           // spotted station, uppercase
    double frequency_khz = 0;
    QString comment;
    QDateTime time;       // UTC; the line carries only HHMMZ, so the date comes from `now_utc`
    QString grid;         // optional trailing locator, else empty
    QString band;         // "20m" etc., empty when outside the amateur bands
    QString mode;         // CW/SSB/FT8/FT4/RTTY when named in the comment, else empty
    QString ota_program;  // IOTA/POTA/SOTA/WWFF when the comment names a reference, else empty
    QString ota_ref;      // e.g. "EU-005", "K-1234", "W7W/KG-001", "KFF-1234"
};

// Returns nothing for any line that is not a well-formed spot.
std::optional<DxSpot> parse_spot_line(const QString& line, const QDateTime& now_utc);
QString band_for_khz(double khz);

struct OtaRef { QString program, ref; };
// First on-the-air reference named in a spot comment (WWFF, SOTA, IOTA, POTA, in that order); empty when none.
OtaRef parse_ota_ref(const QString& comment);

// IOTA "groups.json" (iota-world.org developer download) -> reference number to group name, e.g. "EU-005" -> "British Isles".
// Entries without a well-formed refno or a name are dropped; names are clipped to 60 characters. Empty on malformed input.
QHash<QString, QString> parse_iota_groups(const QByteArray& json);

} // namespace hamclock::dxcluster
