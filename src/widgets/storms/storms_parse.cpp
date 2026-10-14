#include "storms_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimeZone>
#include <algorithm>
#include <cmath>

namespace hamclock::storms {
namespace {
// NHC sends numbers as strings ("35") but some fields as numbers; accept both.
bool number(const QJsonValue& v, double* out) {
    if (v.isDouble()) { *out = v.toDouble(); return true; }
    if (v.isString()) { bool ok = false; *out = v.toString().toDouble(&ok); return ok; }
    return false;
}
} // namespace

Feed parse(const QByteArray& body) {
    Feed feed;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject() || !doc.object().value("activeStorms").isArray()) return feed;
    feed.ok = true;
    for (const auto& item : doc.object().value("activeStorms").toArray()) {
        const auto o = item.toObject();
        Storm s;
        double wind = 0, lat = 0, lon = 0, v = 0;
        s.name = o.value("name").toString().trimmed();
        if (s.name.isEmpty() || !number(o.value("intensity"), &wind) || wind < 0 ||
            !number(o.value("latitudeNumeric"), &lat) || !number(o.value("longitudeNumeric"), &lon) ||
            std::abs(lat) > 90 || std::abs(lon) > 180)
            continue;
        s.id = o.value("id").toString();
        s.classification = o.value("classification").toString();
        s.wind_kt = static_cast<int>(std::lround(wind));
        if (number(o.value("pressure"), &v) && v > 0) s.pressure_mb = static_cast<int>(std::lround(v));
        s.latitude = lat;
        s.longitude = lon;
        if (number(o.value("movementDir"), &v) && v >= 0 && v <= 360) s.heading_deg = static_cast<int>(std::lround(v)) % 360;
        if (number(o.value("movementSpeed"), &v) && v >= 0) s.speed_kt = static_cast<int>(std::lround(v));
        const auto adv = o.value("publicAdvisory").toObject();
        s.issued = QDateTime::fromString(adv.value("issuance").toString(), Qt::ISODate).toUTC();
        if (!s.issued.isValid()) s.issued = QDateTime::fromString(o.value("lastUpdate").toString(), Qt::ISODate).toUTC();
        s.url = adv.value("url").toString();
        const auto fc = o.value("forecastAdvisory").toObject();
        s.forecast_url = fc.value("url").toString();
        s.forecast_adv = fc.value("advNum").toString();
        feed.storms.push_back(s);
    }
    std::stable_sort(feed.storms.begin(), feed.storms.end(),
                     [](const Storm& a, const Storm& b) { return a.wind_kt > b.wind_kt; });
    return feed;
}

Forecast parse_forecast(const QByteArray& body, const QDateTime& issued) {
    Forecast fc;
    const QString text = QString::fromLatin1(body);
    static const QRegularExpression adv_number(R"(FORECAST/ADVISORY NUMBER\s+(\d+))");
    static const QRegularExpression id(R"(\b([A-Z]{2}\d{6})\b)");
    static const QRegularExpression point(
        R"((FORECAST|OUTLOOK) VALID (\d{2})/(\d{2})(\d{2})Z\s+(\d+(?:\.\d+)?)([NS])\s+(\d+(?:\.\d+)?)([EW])\s*\n\s*MAX WIND\s+(\d+) KT)");
    const auto n = adv_number.match(text);
    if (!n.hasMatch() || !issued.isValid()) return fc;
    fc.advisory = n.captured(1).toInt();
    fc.storm_id = id.match(text).captured(1);
    const QDate base = issued.toUTC().date();
    for (auto it = point.globalMatch(text); it.hasNext();) {
        const auto m = it.next();
        const int day = m.captured(2).toInt(), hour = m.captured(3).toInt(), min = m.captured(4).toInt();
        // The stamp has no month: take the issuance month, or the next when the day has wrapped.
        QDate date(base.year(), base.month(), 1);
        if (day < base.day() - 15) date = date.addMonths(1);
        date = QDate(date.year(), date.month(), day);
        ForecastPoint p;
        p.latitude = m.captured(5).toDouble() * (m.captured(6) == "S" ? -1 : 1);
        p.longitude = m.captured(7).toDouble() * (m.captured(8) == "W" ? -1 : 1);
        if (!date.isValid() || hour > 23 || min > 59 || std::abs(p.latitude) > 90 || std::abs(p.longitude) > 180) continue;
        p.valid = QDateTime(date, QTime(hour, min), QTimeZone::utc());
        p.wind_kt = m.captured(9).toInt();
        p.outlook = m.captured(1) == "OUTLOOK";
        fc.points.push_back(p);
    }
    fc.ok = !fc.points.empty();
    return fc;
}

QString classification_text(const QString& code) {
    if (code == "HU") return "Hurricane";
    if (code == "TS") return "Tropical storm";
    if (code == "TD") return "Tropical depression";
    if (code == "STS") return "Subtropical storm";
    if (code == "STD") return "Subtropical depression";
    if (code == "PTC") return "Potential cyclone";
    if (code == "PC") return "Post-tropical cyclone";
    if (code == "RE") return "Remnants";
    return "Tropical cyclone";
}

int category(int wind_kt) {
    if (wind_kt < 64) return 0;
    if (wind_kt < 83) return 1;
    if (wind_kt < 96) return 2;
    if (wind_kt < 113) return 3;
    if (wind_kt < 137) return 4;
    return 5;
}

QString compass(int degrees) {
    if (degrees < 0) return {};
    static const char* const points[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    return points[((degrees % 360) * 8 + 180) / 360 % 8];
}

} // namespace hamclock::storms
