#include "astro_transmitters.hpp"
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace hamclock {

std::vector<Transmitter> parse_satnogs_transmitters(const std::string& json) {
    std::vector<Transmitter> out;
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!doc.isArray()) return out;
    static const QRegularExpression tone(QStringLiteral("CTCSS\\s*([0-9]+(?:\\.[0-9]+)?)"));
    for (const QJsonValue& v : doc.array()) {
        const QJsonObject o = v.toObject();
        Transmitter t;
        t.norad_id = o.value(QStringLiteral("norad_cat_id")).toInt();
        t.downlink_low_hz = o.value(QStringLiteral("downlink_low")).toDouble();
        t.downlink_high_hz = o.value(QStringLiteral("downlink_high")).toDouble();
        t.uplink_low_hz = o.value(QStringLiteral("uplink_low")).toDouble();
        t.uplink_high_hz = o.value(QStringLiteral("uplink_high")).toDouble();
        if (t.norad_id <= 0 || (t.downlink_low_hz <= 0 && t.uplink_low_hz <= 0)) continue;
        const QString desc = o.value(QStringLiteral("description")).toString();
        t.description = desc.toStdString();
        t.mode = o.value(QStringLiteral("mode")).toString().toStdString();
        t.uplink_mode = o.value(QStringLiteral("uplink_mode")).toString().toStdString();
        t.inverting = o.value(QStringLiteral("invert")).toBool();
        t.active = o.value(QStringLiteral("status")).toString() == QLatin1String("active");
        const auto m = tone.match(desc);
        if (m.hasMatch()) t.ctcss_hz = m.captured(1).toDouble();
        out.push_back(std::move(t));
    }
    return out;
}

int tle_norad_id(const Tle& tle) {
    if (tle.line1.size() < 7) return 0;
    int n = 0;
    for (int i = 2; i < 7; ++i) {
        const char c = tle.line1[static_cast<std::size_t>(i)];
        if (c == ' ') continue;
        if (c < '0' || c > '9') return 0;
        n = n * 10 + (c - '0');
    }
    return n;
}

std::map<int, std::vector<Transmitter>> group_by_norad(const std::vector<Transmitter>& all) {
    std::map<int, std::vector<Transmitter>> m;
    for (const auto& t : all) m[t.norad_id].push_back(t);
    return m;
}

double transponder_downlink_hz(const Transmitter& t, double uplink_hz) {
    if (!t.transponder() || uplink_hz < t.uplink_low_hz || uplink_hz > t.uplink_high_hz) return 0;
    return t.inverting ? t.downlink_low_hz + (t.uplink_high_hz - uplink_hz)
                       : t.downlink_low_hz + (uplink_hz - t.uplink_low_hz);
}

} // namespace hamclock
