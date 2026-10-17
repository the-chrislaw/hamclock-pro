#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "dx_spot_parse.hpp"
#include <QRegularExpression>
#include <QTimeZone>
#include <cmath>
#include <utility>

namespace hamclock::dxcluster {

QString band_for_khz(double khz) {
    static const struct { double lo, hi; const char* name; } bands[] = {
        {1800, 2000, "160m"}, {3500, 4000, "80m"}, {5250, 5450, "60m"}, {7000, 7300, "40m"},
        {10100, 10150, "30m"}, {14000, 14350, "20m"}, {18068, 18168, "17m"}, {21000, 21450, "15m"},
        {24890, 24990, "12m"}, {28000, 29700, "10m"}, {50000, 54000, "6m"}, {144000, 148000, "2m"},
        {430000, 450000, "70cm"}};
    for (const auto& b : bands)
        if (khz >= b.lo && khz <= b.hi) return b.name;
    return {};
}

OtaRef parse_ota_ref(const QString& comment) {
    static const struct { const char* program; QRegularExpression re; } rules[] = {
        {"WWFF", QRegularExpression(R"((?<![A-Z0-9/-])([A-Z0-9]{1,2}FF-\d{4})(?![A-Z0-9-]))", QRegularExpression::CaseInsensitiveOption)},
        {"SOTA", QRegularExpression(R"((?<![A-Z0-9/-])([A-Z0-9]{1,3}/[A-Z]{2}-\d{3})(?![A-Z0-9-]))", QRegularExpression::CaseInsensitiveOption)},
        {"IOTA", QRegularExpression(R"((?<![A-Z0-9/-])((?:AF|AN|AS|EU|NA|OC|SA)-\d{3})(?![A-Z0-9-]))", QRegularExpression::CaseInsensitiveOption)},
        {"POTA", QRegularExpression(R"((?<![A-Z0-9/-])([A-Z]{1,2}-\d{4,5})(?![A-Z0-9-]))", QRegularExpression::CaseInsensitiveOption)}};
    for (const auto& r : rules) {
        const auto m = r.re.match(comment);
        if (m.hasMatch()) return {r.program, m.captured(1).toUpper()};
    }
    return {};
}

std::optional<DxSpot> parse_spot_line(const QString& line, const QDateTime& now_utc) {
    static const QRegularExpression re(
        R"(^DX de ([A-Z0-9/#-]{3,16}):?\s+(\d{3,7}\.\d{1,2})\s+([A-Z0-9/]{3,16})\s+(.*?)\s*(\d{2})(\d{2})Z(?:\s+([A-R]{2}\d{2}(?:[A-X]{2})?))?\s*$)",
        QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(line.trimmed());
    if (!m.hasMatch()) return std::nullopt;
    const int hour = m.captured(5).toInt(), minute = m.captured(6).toInt();
    if (hour > 23 || minute > 59) return std::nullopt;
    DxSpot spot;
    spot.spotter = m.captured(1).toUpper();
    spot.frequency_khz = m.captured(2).toDouble();
    spot.dx = m.captured(3).toUpper();
    spot.comment = m.captured(4).trimmed();
    spot.grid = m.captured(7).toUpper();
    if (!std::isfinite(spot.frequency_khz) || spot.frequency_khz <= 0) return std::nullopt;
    spot.band = band_for_khz(spot.frequency_khz);
    // The line has no date: assume today (UTC), or yesterday when that would be in the future.
    QDateTime t(now_utc.date(), QTime(hour, minute), QTimeZone::utc());
    if (t > now_utc.addSecs(300)) t = t.addDays(-1);
    spot.time = t;
    static const QRegularExpression mode_re(R"(\b(CW|SSB|FT8|FT4|RTTY|PSK\d*)\b)", QRegularExpression::CaseInsensitiveOption);
    const auto mm = mode_re.match(spot.comment);
    if (mm.hasMatch()) spot.mode = mm.captured(1).toUpper();
    const OtaRef ota = parse_ota_ref(spot.comment);
    spot.ota_program = ota.program;
    spot.ota_ref = ota.ref;
    return spot;
}

QHash<QString, QString> parse_iota_groups(const QByteArray& json) {
    QHash<QString, QString> out;
    const auto doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) return out;
    static const QRegularExpression ref_re(R"(^(?:AF|AN|AS|EU|NA|OC|SA)-\d{3}$)");
    for (const auto& v : doc.array()) {
        const auto o = v.toObject();
        const QString ref = o.value("refno").toString().trimmed().toUpper();
        const QString name = o.value("name").toString().trimmed().left(60);
        if (ref_re.match(ref).hasMatch() && !name.isEmpty()) out.insert(ref, name);
    }
    return out;
}

} // namespace hamclock::dxcluster
