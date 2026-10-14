#include "marine_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace hamclock::marine {
namespace {
struct EventRank { const char* name; int rank; };
constexpr EventRank kEvents[] = {
    {"Hurricane Force Wind Warning", 5}, {"Storm Warning", 4}, {"Special Marine Warning", 4},
    {"Hazardous Seas Warning", 3},       {"Gale Warning", 3},  {"Small Craft Advisory", 1},
    {"Brisk Wind Advisory", 1},          {"Low Water Advisory", 1},
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

QString region_for(double lat, double lon) {
    if (lat >= 51 && lon <= -130) return "AL";
    if ((lat < 30 && lon < -150) || lon > 130 || lon < -165) return "PI";
    if (lat >= 41.3 && lat <= 49.5 && lon >= -92.5 && lon <= -76) return "GL";
    if (lat < 31.5 && lon >= -98 && lon <= -81.8) return "GM";
    if (lon <= -114 && lat >= 32 && lat <= 49.5) return "PA";
    if (lon >= -82 && lon <= -50 && lat >= 24 && lat <= 50) return "AT";
    return {};
}

QString region_name(const QString& code) {
    if (code == "GM") return "Gulf of Mexico";
    if (code == "AT") return "Atlantic";
    if (code == "PA") return "Pacific coast";
    if (code == "AL") return "Alaska";
    if (code == "PI") return "Pacific islands";
    if (code == "GL") return "Great Lakes";
    return {};
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
        feed.alerts.push_back(a);
    }
    std::stable_sort(feed.alerts.begin(), feed.alerts.end(), [](const Alert& a, const Alert& b) {
        return a.rank != b.rank ? a.rank > b.rank : a.expires < b.expires;
    });
    return feed;
}

} // namespace hamclock::marine
