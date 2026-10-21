#include "hamsat_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace hamclock::hamsat {

Feed parse(const QByteArray& body) {
    Feed feed;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject() || !doc.object().value("data").isArray()) return feed;
    feed.ok = true;
    for (const auto& item : doc.object().value("data").toArray()) {
        const auto o = item.toObject();
        Activation a;
        a.callsign = o.value("callsign").toString();
        a.satellite = o.value("satellite").toObject().value("name").toString();
        a.aos = QDateTime::fromString(o.value("aos_at").toString(), Qt::ISODate).toUTC();
        a.los = QDateTime::fromString(o.value("los_at").toString(), Qt::ISODate).toUTC();
        if (a.callsign.isEmpty() || a.satellite.isEmpty() || !a.aos.isValid() || !a.los.isValid()) continue;
        a.mode = o.value("mode").toString();
        QStringList grids;
        for (const auto& g : o.value("grids").toArray()) grids << g.toString();
        a.grids = grids.join(' ');
        a.mhz = o.value("mhz").toDouble(0);
        if (o.value("is_workable").isBool()) a.workable = o.value("is_workable").toBool() ? 1 : 0;
        if (o.value("max_elevation").isDouble()) a.max_elevation = o.value("max_elevation").toDouble();
        if (o.value("workable_start_at").isString())
            a.workable_start = QDateTime::fromString(o.value("workable_start_at").toString(), Qt::ISODate).toUTC();
        if (o.value("workable_end_at").isString())
            a.workable_end = QDateTime::fromString(o.value("workable_end_at").toString(), Qt::ISODate).toUTC();
        feed.activations.push_back(a);
    }
    std::stable_sort(feed.activations.begin(), feed.activations.end(),
                     [](const Activation& x, const Activation& y) { return x.aos < y.aos; });
    return feed;
}

std::vector<Activation> upcoming(const Feed& feed, const QDateTime& now, int limit) {
    std::vector<Activation> out;
    for (const auto& a : feed.activations) {
        if (a.los <= now) continue;
        if (static_cast<int>(out.size()) >= limit) break;
        out.push_back(a);
    }
    return out;
}

QString when_text(const Activation& a, const QDateTime& now) {
    const qint64 secs = now.secsTo(a.aos);
    if (secs <= 0) return "now";
    const qint64 mins = secs / 60;
    if (mins >= 2880) return QString("in %1d %2h").arg(mins / 1440).arg(mins % 1440 / 60);
    if (mins >= 60) return QString("in %1h %2m").arg(mins / 60).arg(mins % 60);
    return QString("in %1m").arg(mins);
}

} // namespace hamclock::hamsat
