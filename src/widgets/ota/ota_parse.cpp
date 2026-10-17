#include "ota_parse.hpp"
#include "../dxcluster/dx_spot_parse.hpp"
#include "../../domain/geometry.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTimeZone>
#include <algorithm>
#include <cmath>

namespace hamclock::ota {
namespace {

bool is_qrt(const QString& comment) {
    return comment.contains(QStringLiteral("qrt"), Qt::CaseInsensitive);
}

double number(const QJsonValue& v) {
    if (v.isDouble()) return v.toDouble();
    if (v.isString()) {
        bool ok = false;
        const double d = v.toString().toDouble(&ok);
        return ok ? d : 0;
    }
    return 0;
}

QDateTime utc_time(const QString& text) {
    // POTA gives "2026-10-13T17:39:39" (UTC, no zone); SOTA a zoned ISO string.
    auto t = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (!t.isValid()) t = QDateTime::fromString(text, Qt::ISODate);
    if (!t.isValid()) return {};
    if (t.timeSpec() != Qt::UTC && t.timeSpec() != Qt::OffsetFromUTC) t.setTimeZone(QTimeZone::utc());
    return t.toUTC();
}

void set_position(Spot& s, const QJsonObject& o) {
    const auto lat = o.value("latitude"), lon = o.value("longitude");
    if (!lat.isDouble() || !lon.isDouble()) return;
    s.latitude = lat.toDouble();
    s.longitude = lon.toDouble();
    s.has_position = std::fabs(s.latitude) <= 90 && std::fabs(s.longitude) <= 180 &&
                     !(s.latitude == 0 && s.longitude == 0);
}

void sort_newest(std::vector<Spot>& spots) {
    std::stable_sort(spots.begin(), spots.end(), [](const Spot& a, const Spot& b) { return a.time > b.time; });
}

} // namespace

Feed parse_pota(const QByteArray& body) {
    Feed feed;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isArray()) return feed;
    feed.ok = true;
    for (const auto& item : doc.array()) {
        const auto o = item.toObject();
        Spot s;
        s.program = "POTA";
        s.call = o.value("activator").toString().trimmed().toUpper();
        s.reference = o.value("reference").toString().trimmed();
        s.name = o.value("name").toString();
        s.freq_khz = number(o.value("frequency"));
        s.mode = o.value("mode").toString().toUpper();
        s.time = utc_time(o.value("spotTime").toString());
        s.comment = o.value("comments").toString();
        set_position(s, o);
        if (s.call.isEmpty() || s.reference.isEmpty() || s.freq_khz <= 0 || !s.time.isValid() || is_qrt(s.comment))
            continue;
        feed.spots.push_back(s);
    }
    sort_newest(feed.spots);
    return feed;
}

Feed parse_sota(const QByteArray& body) {
    Feed feed;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isArray()) return feed;
    feed.ok = true;
    for (const auto& item : doc.array()) {
        const auto o = item.toObject();
        Spot s;
        s.program = "SOTA";
        s.call = o.value("activatorCallsign").toString().trimmed().toUpper();
        s.reference = o.value("summitCode").toString().trimmed();
        s.name = o.value("summitName").toString();
        s.freq_khz = number(o.value("frequency")) * 1000.0;
        s.mode = o.value("mode").toString().toUpper();
        s.time = utc_time(o.value("timeStamp").toString());
        s.comment = o.value("comments").toString();
        set_position(s, o);
        if (s.call.isEmpty() || s.reference.isEmpty() || s.freq_khz <= 0 || !s.time.isValid() || is_qrt(s.comment))
            continue;
        feed.spots.push_back(s);
    }
    sort_newest(feed.spots);
    return feed;
}

Feed parse_wwff(const QByteArray& body) {
    Feed feed;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isArray()) return feed;
    feed.ok = true;
    for (const auto& item : doc.array()) {
        const auto o = item.toObject();
        Spot s;
        s.program = "WWFF";
        s.call = o.value("activator").toString().trimmed().toUpper();
        s.reference = o.value("reference").toString().trimmed();
        s.name = o.value("reference_name").toString();
        s.freq_khz = number(o.value("frequency_khz"));
        s.mode = o.value("mode").toString().toUpper();
        const double epoch = number(o.value("spot_time"));
        if (epoch > 0) s.time = QDateTime::fromSecsSinceEpoch(qint64(epoch), QTimeZone::utc());
        s.comment = o.value("remarks").toString();
        set_position(s, o);
        if (s.call.isEmpty() || s.reference.isEmpty() || s.freq_khz <= 0 || !s.time.isValid() || is_qrt(s.comment))
            continue;
        feed.spots.push_back(s);
    }
    sort_newest(feed.spots);
    return feed;
}

std::vector<Spot> merge(const Feed& pota, const Feed& sota, const Feed& wwff) {
    std::vector<Spot> all = pota.spots;
    all.insert(all.end(), sota.spots.begin(), sota.spots.end());
    all.insert(all.end(), wwff.spots.begin(), wwff.spots.end());
    sort_newest(all);
    std::vector<Spot> out;
    QSet<QString> seen;
    for (const auto& s : all) {
        const auto key = s.program + '|' + s.call + '|' + s.reference;
        if (seen.contains(key)) continue;
        seen.insert(key);
        out.push_back(s);
    }
    return out;
}

QString format_frequency(double khz) { return QString::number(khz / 1000.0, 'f', 3); }

std::vector<Spot> filter(const std::vector<Spot>& spots, const Filter& f, const QDateTime& now) {
    std::vector<Spot> out;
    for (const auto& s : spots) {
        if (!f.program.isEmpty() && s.program != f.program) continue;
        if (!f.mode.isEmpty() && s.mode.compare(f.mode, Qt::CaseInsensitive) != 0) continue;
        if (!f.band.isEmpty() && dxcluster::band_for_khz(s.freq_khz) != f.band) continue;
        if (f.max_age_min > 0 && s.time.secsTo(now) > qint64(f.max_age_min) * 60) continue;
        out.push_back(s);
    }
    if (f.sort == "freq") {
        std::stable_sort(out.begin(), out.end(), [](const Spot& a, const Spot& b) { return a.freq_khz < b.freq_khz; });
    } else if (f.sort == "dist" && f.has_de && valid_location({f.de_lat, f.de_lon})) {
        const auto dist = [&](const Spot& s) {
            return s.has_position ? geometry({f.de_lat, f.de_lon}, {s.latitude, s.longitude}).distance_km : 1e12;
        };
        std::stable_sort(out.begin(), out.end(), [&](const Spot& a, const Spot& b) { return dist(a) < dist(b); });
    }
    return out;
}

} // namespace hamclock::ota
