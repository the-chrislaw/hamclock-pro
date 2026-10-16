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

std::optional<PadPosition> pad_position(const Launch& l) {
    struct Site { const char* key; double lat; double lon; };
    static const Site sites[] = {
        {"Cape Canaveral", 28.49, -80.57}, {"Kennedy Space Center", 28.57, -80.65},
        {"Vandenberg", 34.73, -120.57},    {"Starbase", 25.99, -97.15},
        {"Boca Chica", 25.99, -97.15},     {"Wallops", 37.94, -75.47},
        {"Mahia", -39.26, 177.86},         {"Kodiak", 57.44, -152.34},
        {"Kourou", 5.24, -52.77},          {"Baikonur", 45.96, 63.31},
        {"Plesetsk", 62.93, 40.58},        {"Vostochny", 51.88, 128.33},
        {"Jiuquan", 40.96, 100.28},        {"Taiyuan", 38.85, 111.61},
        {"Xichang", 28.25, 102.03},        {"Wenchang", 19.61, 110.95},
        {"Tanegashima", 30.40, 130.97},    {"Uchinoura", 31.25, 131.08},
        {"Satish Dhawan", 13.73, 80.23},   {"Sriharikota", 13.73, 80.23},
        {"Naro", 34.43, 127.54},           {"Semnan", 35.23, 53.92},
        {"Andoya", 69.29, 16.02},          {"Esrange", 67.89, 21.10},
    };
    for (const auto& s : sites)
        if (l.location.contains(QLatin1String(s.key), Qt::CaseInsensitive)) return PadPosition{s.lat, s.lon};
    return std::nullopt;
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
