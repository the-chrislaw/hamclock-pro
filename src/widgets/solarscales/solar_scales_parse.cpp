#include "solar_scales_parse.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

namespace hamclock::solarscales {
namespace {
// SWPC sends numbers as strings, or null when a record has no value.
int number(const QJsonValue& v, int max) {
    const QString text = v.isString() ? v.toString() : v.isDouble() ? QString::number(v.toInt(-1)) : QString();
    bool ok = false;
    const int n = text.toInt(&ok);
    return ok && n >= 0 && n <= max ? n : -1;
}

Day parse_day(const QJsonObject& rec) {
    Day d;
    const auto r = rec.value("R").toObject(), s = rec.value("S").toObject(), g = rec.value("G").toObject();
    d.r = number(r.value("Scale"), 5);
    d.s = number(s.value("Scale"), 5);
    d.g = number(g.value("Scale"), 5);
    d.r_minor = number(r.value("MinorProb"), 100);
    d.r_major = number(r.value("MajorProb"), 100);
    d.s_prob = number(s.value("Prob"), 100);
    return d;
}
} // namespace

Scales parse_scales(const QByteArray& body) {
    Scales out;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) return out;
    const auto root = doc.object();
    if (!root.value("0").isObject()) return out;
    const auto now = root.value("0").toObject();
    out.current = parse_day(now);
    if (out.current.r < 0 || out.current.s < 0 || out.current.g < 0) return out;  // no current state to show
    auto when = QDateTime::fromString(now.value("DateStamp").toString() + " " + now.value("TimeStamp").toString(),
                                      "yyyy-MM-dd HH:mm:ss");
    if (!when.isValid()) return out;
    when.setTimeZone(QTimeZone::utc());
    out.observed = when;
    if (root.value("-1").isObject()) out.past24h = parse_day(root.value("-1").toObject());
    // Day 1 of the feed is the rest of today, 2 and 3 the following days.
    for (int i = 0; i < 3; ++i) {
        const auto rec = root.value(QString::number(i + 1));
        if (rec.isObject()) out.forecast[static_cast<size_t>(i)] = parse_day(rec.toObject());
    }
    out.valid = true;
    return out;
}

QString scale_word(int level) {
    static const char* words[] = {"", "Minor", "Moderate", "Strong", "Severe", "Extreme"};
    return level >= 1 && level <= 5 ? words[level] : QString();
}

} // namespace hamclock::solarscales
