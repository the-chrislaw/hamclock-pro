#include "solar_dst_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <algorithm>

namespace hamclock::solardst {

std::vector<Sample> parse_dst(const QByteArray& body) {
    std::vector<Sample> out;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isArray()) return out;
    for (const auto& v : doc.array()) {
        const auto rec = v.toObject();
        const auto value = rec.value("dst");
        if (!value.isDouble()) continue;  // null or text: no reading for that hour
        auto time = QDateTime::fromString(rec.value("time_tag").toString(), "yyyy-MM-dd'T'HH:mm:ss");
        if (!time.isValid()) continue;
        time.setTimeZone(QTimeZone::utc());
        out.push_back({time, qRound(value.toDouble())});
    }
    std::sort(out.begin(), out.end(), [](const Sample& a, const Sample& b) { return a.time < b.time; });
    return out;
}

QString dst_word(int dst) {
    if (dst > -30) return "Quiet";
    if (dst > -50) return "Weak";
    if (dst > -100) return "Moderate";
    if (dst >= -250) return "Intense";
    return "Super";
}

} // namespace hamclock::solardst
