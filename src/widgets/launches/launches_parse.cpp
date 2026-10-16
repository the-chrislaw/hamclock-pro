#include "launches_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <algorithm>

namespace hamclock::launches {

Feed parse(const QByteArray& body) {
    Feed feed;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject() || !doc.object().value("results").isArray()) return feed;
    feed.ok = true;
    for (const auto& item : doc.object().value("results").toArray()) {
        const auto o = item.toObject();
        Launch l;
        l.name = o.value("name").toString();
        l.net = QDateTime::fromString(o.value("net").toString(), Qt::ISODate).toUTC();
        if (l.name.isEmpty() || !l.net.isValid()) continue;
        l.provider = o.value("lsp_name").toString();
        l.mission = o.value("mission").toString();
        l.mission_type = o.value("mission_type").toString();
        l.pad = o.value("pad").toString();
        l.location = o.value("location").toString();
        l.status = o.value("status").toObject().value("abbrev").toString();
        const auto prec = o.value("net_precision").toObject().value("abbrev").toString();
        l.precise = prec == "MIN" || prec == "SEC" || prec == "HR";
        feed.launches.push_back(l);
    }
    std::stable_sort(feed.launches.begin(), feed.launches.end(),
                     [](const Launch& a, const Launch& b) { return a.net < b.net; });
    return feed;
}

QString ham_hint(const Launch& l) {
    const QString text = l.name + ' ' + l.mission + ' ' + l.mission_type;
    auto has = [&](const char* w) { return text.contains(QLatin1String(w), Qt::CaseInsensitive); };
    if (has("amateur") || has("oscar") || has("cubesat")) return "Amateur/cubesat payloads";
    if (has("rideshare") || has("transporter") || has("bandwagon")) return "Rideshare: possible amateur sats";
    if (has("international space station") || has("iss ") || has("crew-") || has("cygnus") || has("progress ms") ||
        has("soyuz ms") || has("CRS-"))
        return "ISS visit (ARISS)";
    return {};
}

QString countdown_text(const QDateTime& net, const QDateTime& now) {
    const qint64 secs = now.secsTo(net);
    if (secs <= 0) return "T+0m";
    const qint64 mins = secs / 60;
    if (mins >= 2880) return QString("T-%1d %2h").arg(mins / 1440).arg(mins % 1440 / 60);
    if (mins >= 60) return QString("T-%1h %2m").arg(mins / 60).arg(mins % 60);
    return QString("T-%1m").arg(mins);
}

std::vector<Launch> upcoming(const Feed& feed, const QDateTime& now, int limit) {
    std::vector<Launch> out;
    const auto from = now.addSecs(-2 * 3600);
    for (const auto& l : feed.launches) {
        if (l.net < from) continue;
        if (static_cast<int>(out.size()) >= limit) break;
        out.push_back(l);
    }
    return out;
}

} // namespace hamclock::launches
