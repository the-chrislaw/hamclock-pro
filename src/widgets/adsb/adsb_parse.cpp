#include "adsb_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

namespace hamclock::adsb {

QString aircraft_url(const QString& host, const QString& port) {
    QString h = host.trimmed();
    bool hostOk = !h.isEmpty() && h.size() <= 253;
    for (const QChar c : h)
        if (!(c.isLetterOrNumber() && c.unicode() < 128) && c != '.' && c != '-' && c != ':') hostOk = false;
    if (!hostOk) h = "127.0.0.1";
    else if (h.contains(':')) h = "[" + h + "]";
    bool ok = false;
    uint p = port.trimmed().toUInt(&ok);
    if (!ok || p < 1 || p > 65535) p = 8080;
    return QString("http://%1:%2/data/aircraft.json").arg(h).arg(p);
}

std::optional<Summary> parse_aircraft(const QByteArray& json) {
    const auto doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) return std::nullopt;
    const auto obj = doc.object();
    const auto now = obj.value("now");
    const auto list = obj.value("aircraft");
    if (!now.isDouble() || !list.isArray() || !std::isfinite(now.toDouble())) return std::nullopt;
    Summary s;
    s.now = now.toDouble();
    for (const auto& v : list.toArray()) {
        const auto a = v.toObject();
        if (a.value("seen").toDouble(0) > 60) continue;
        ++s.tracked;
        if (a.value("lat").isDouble() && a.value("lon").isDouble()) ++s.withPosition;
    }
    return s;
}

} // namespace hamclock::adsb
