#include "gps_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <cstdlib>

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
        const auto t = QDateTime::fromString(o.value("time").toString(), Qt::ISODateWithMs);
        if (t.isValid() && t.date().year() >= 2020) m.fix.time = t.toUTC();
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

QString clock_vs_gps_text(const QDateTime& gps_utc, const QDateTime& system_utc) {
    const qint64 ms = system_utc.toMSecsSinceEpoch() - gps_utc.toMSecsSinceEpoch();
    if (std::llabs(ms) < 2000) return "Clock agrees with GPS (within 2 s)";
    const long long s = std::llround(std::fabs(ms / 1000.0));
    const QString span = s >= 3600 ? QString("%1 h").arg(s / 3600.0, 0, 'f', 1) : QString("%1 s").arg(s);
    return QString("Clock is %1 %2 GPS").arg(span, ms > 0 ? "ahead of" : "behind");
}

} // namespace hamclock::gps
