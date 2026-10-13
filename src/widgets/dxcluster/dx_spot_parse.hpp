#pragma once
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
};

// Returns nothing for any line that is not a well-formed spot.
std::optional<DxSpot> parse_spot_line(const QString& line, const QDateTime& now_utc);
QString band_for_khz(double khz);

} // namespace hamclock::dxcluster
