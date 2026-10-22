#include "balloons_parse.hpp"
#include "../../domain/geometry.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

#include <cmath>

namespace hamclock::balloons {

Feed parse(const QByteArray& body) {
    Feed feed;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) return feed;
    feed.ok = true;
    const auto root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        const auto frames = it.value().toObject();
        Sonde best;
        Sonde prev;
        for (const auto& fv : frames) {
            const auto f = fv.toObject();
            if (!f.value("lat").isDouble() || !f.value("lon").isDouble()) continue;
            const auto time = QDateTime::fromString(f.value("datetime").toString(), Qt::ISODate).toUTC();
            if (!time.isValid() || (best.time.isValid() && time <= best.time)) continue;
            Sonde s;
            s.serial = f.value("serial").toString(it.key());
            s.type = f.value("type").toString();
            s.lat = f.value("lat").toDouble();
            s.lon = f.value("lon").toDouble();
            if (!valid_location({s.lat, s.lon})) continue;
            s.alt_m = f.value("alt").toDouble();
            s.has_vel_v = f.value("vel_v").isDouble();
            s.vel_v = f.value("vel_v").toDouble();
            s.freq_mhz = f.value("frequency").toDouble();
            s.time = time;
            prev = best;
            best = s;
        }
        if (best.time.isValid() && prev.time.isValid()) {
            best.has_prev = true;
            best.prev_lat = prev.lat; best.prev_lon = prev.lon; best.prev_time = prev.time;
        }
        if (best.time.isValid()) feed.sondes.push_back(best);
    }
    return feed;
}

QString phase(const Sonde& s) {
    if (!s.has_vel_v || qAbs(s.vel_v) < 1.0) return {};
    return s.vel_v > 0 ? "climbing" : "descending";
}

bool landing_point(const Sonde& s, qint64 age_s, double& lat, double& lon) {
    const int left = landing_eta_remaining_s(s, age_s);
    if (left <= 0 || !s.has_prev) return false;
    const double dt = s.prev_time.msecsTo(s.time) / 1000.0;
    if (dt <= 0 || dt > 600) return false;
    // Frames are seconds apart, so a flat-earth step is accurate enough.
    const double dlat = (s.lat - s.prev_lat) / dt;
    double dlon = s.lon - s.prev_lon;
    if (dlon > 180) dlon -= 360; else if (dlon < -180) dlon += 360;
    dlon /= dt;
    lat = s.lat + dlat * left;
    lon = s.lon + dlon * left;
    if (lat > 90 || lat < -90) return false;
    while (lon > 180) lon -= 360;
    while (lon < -180) lon += 360;
    return true;
}

std::vector<Near> nearest(const Feed& feed, double lat, double lon, const QDateTime& now, int max_age_s, int limit) {
    std::vector<Near> out;
    for (const auto& s : feed.sondes) {
        const auto age = s.time.secsTo(now);
        if (age > max_age_s || age < -600) continue;
        const auto g = geometry({lat, lon}, {s.lat, s.lon});
        out.push_back({s, g.distance_km, g.short_bearing_deg});
    }
    std::sort(out.begin(), out.end(), [](const Near& a, const Near& b) { return a.distance_km < b.distance_km; });
    if (static_cast<int>(out.size()) > limit) out.resize(limit);
    return out;
}

int landing_eta_s(const Sonde& s) {
    if (!s.has_vel_v || s.vel_v > -2.0 || s.alt_m < 0) return -1;
    constexpr double kScaleHeightM = 7500.0;
    const double h2 = 2.0 * kScaleHeightM;
    return static_cast<int>(std::lround(h2 / -s.vel_v * (std::exp(s.alt_m / h2) - 1.0)));
}

int landing_eta_remaining_s(const Sonde& s, qint64 age_s) {
    const int eta = landing_eta_s(s);
    if (eta < 0) return -1;
    return static_cast<int>(std::max<qint64>(0, eta - std::max<qint64>(0, age_s)));
}

} // namespace hamclock::balloons
