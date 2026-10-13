#include "quakes_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <algorithm>
#include <cmath>

namespace hamclock::quakes {

Feed parse(const QByteArray& body) {
    Feed feed;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) return feed;
    const auto root = doc.object();
    if (root.value("type").toString() != "FeatureCollection" || !root.value("features").isArray()) return feed;
    feed.ok = true;
    const auto generated = root.value("metadata").toObject().value("generated");
    if (generated.isDouble())
        feed.generated = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(generated.toDouble()), QTimeZone::utc());
    for (const auto& item : root.value("features").toArray()) {
        const auto props = item.toObject().value("properties").toObject();
        const auto coords = item.toObject().value("geometry").toObject().value("coordinates").toArray();
        const auto mag = props.value("mag");
        const auto time = props.value("time");
        if (!mag.isDouble() || !time.isDouble() || coords.size() < 2 || !coords[0].isDouble() || !coords[1].isDouble())
            continue;
        Quake q;
        q.magnitude = mag.toDouble();
        q.longitude = coords[0].toDouble();
        q.latitude = coords[1].toDouble();
        q.depth_km = coords.size() > 2 && coords[2].isDouble() ? coords[2].toDouble() : 0;
        if (!std::isfinite(q.magnitude) || std::fabs(q.latitude) > 90 || std::fabs(q.longitude) > 180) continue;
        q.time = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(time.toDouble()), QTimeZone::utc());
        q.place = props.value("place").toString();
        q.tsunami = props.value("tsunami").toInt(0) == 1;
        q.alert = props.value("alert").toString();
        q.url = props.value("url").toString();
        feed.quakes.push_back(q);
    }
    std::stable_sort(feed.quakes.begin(), feed.quakes.end(),
                     [](const Quake& a, const Quake& b) { return a.time > b.time; });
    return feed;
}

const Quake* strongest(const Feed& feed) {
    const Quake* best = nullptr;
    for (const auto& q : feed.quakes)
        if (!best || q.magnitude > best->magnitude) best = &q;
    return best;
}

QString magnitude_class(double m) {
    if (!std::isfinite(m)) return {};
    if (m >= 8) return "Great";
    if (m >= 7) return "Major";
    if (m >= 6) return "Strong";
    if (m >= 5) return "Moderate";
    if (m >= 4) return "Light";
    return "Minor";
}

} // namespace hamclock::quakes
