#include "gps_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

namespace hamclock::gps {

namespace {
std::optional<double> number(const QJsonObject& o, const char* key, double lo, double hi) {
    const auto v = o.value(key);
    if (!v.isDouble()) return std::nullopt;
    const double d = v.toDouble();
    return std::isfinite(d) && d >= lo && d <= hi ? std::optional<double>(d) : std::nullopt;
}
}

Message parse_line(const QByteArray& line) {
    Message m;
    const auto doc = QJsonDocument::fromJson(line);
    if (!doc.isObject()) return m;
    const auto o = doc.object();
    const auto cls = o.value("class").toString();
    if (cls == "TPV") {
        m.kind = Message::Tpv;
        m.fix.mode = o.value("mode").toInt(0);
        m.fix.lat = number(o, "lat", -90, 90);
        m.fix.lon = number(o, "lon", -180, 180);
        if (!m.fix.lat || !m.fix.lon) m.fix.lat = m.fix.lon = std::nullopt;
        m.fix.alt_m = number(o, "altMSL", -1000, 100000);
        if (!m.fix.alt_m) m.fix.alt_m = number(o, "alt", -1000, 100000);
    } else if (cls == "SKY") {
        m.kind = Message::SkyReport;
        const auto sats = o.value("satellites").toArray();
        m.sky.seen = sats.size();
        for (const auto& s : sats) if (s.toObject().value("used").toBool()) ++m.sky.used;
        if (sats.isEmpty()) {
            m.sky.used = o.value("uSat").toInt(0);
            m.sky.seen = o.value("nSat").toInt(0);
        }
    }
    return m;
}

} // namespace hamclock::gps
