#include "firewx_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace hamclock::firewx {
namespace {
struct EventRank { const char* name; int rank; };
constexpr EventRank kEvents[] = {
    {"Red Flag Warning", 4}, {"Fire Warning", 4}, {"Extreme Fire Danger", 4}, {"Fire Weather Watch", 3},
};
QDateTime utc(const QJsonValue& v) { return QDateTime::fromString(v.toString(), Qt::ISODate).toUTC(); }
} // namespace

QString requested_events() {
    QStringList names;
    for (const auto& e : kEvents) names << e.name;
    return names.join(',');
}

int event_rank(const QString& event) {
    for (const auto& e : kEvents) if (event == e.name) return e.rank;
    return 0;
}

Feed parse(const QByteArray& body) {
    Feed feed;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject() || !doc.object().value("features").isArray()) return feed;
    feed.ok = true;
    feed.updated = utc(doc.object().value("updated"));
    for (const auto& item : doc.object().value("features").toArray()) {
        const auto p = item.toObject().value("properties").toObject();
        Alert a;
        a.id = p.value("id").toString();
        a.event = p.value("event").toString();
        a.rank = event_rank(a.event);
        a.expires = utc(p.value("ends"));
        if (!a.expires.isValid()) a.expires = utc(p.value("expires"));
        if (a.id.isEmpty() || a.rank == 0 || !a.expires.isValid()) continue;
        a.area = p.value("areaDesc").toString().simplified();
        a.office = p.value("senderName").toString();
        a.sent = utc(p.value("sent"));
        a.onset = utc(p.value("onset"));
        feed.alerts.push_back(a);
    }
    std::stable_sort(feed.alerts.begin(), feed.alerts.end(), [](const Alert& a, const Alert& b) {
        return a.rank != b.rank ? a.rank > b.rank : a.expires < b.expires;
    });
    return feed;
}

} // namespace hamclock::firewx
