#include "adsb_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

namespace hamclock::adsb {

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
