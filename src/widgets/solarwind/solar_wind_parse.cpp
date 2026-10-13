#include "solar_wind_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <algorithm>
#include <cmath>

namespace hamclock::solarwind {

std::vector<Point> series(const QByteArray& body, const char* field) {
    std::vector<Point> out;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isArray()) return out;
    for (const auto& item : doc.array()) {
        const auto row = item.toObject();
        if (row.value("active").toBool(false) == false) continue;
        const auto value = row.value(QLatin1String(field));
        if (!value.isDouble()) continue;
        const double v = value.toDouble();
        if (!std::isfinite(v) || v <= -9999 + 1e-9) continue;
        auto time = QDateTime::fromString(row.value("time_tag").toString(), Qt::ISODate);
        if (!time.isValid()) continue;
        time.setTimeZone(QTimeZone::utc());
        out.push_back({time, v});
    }
    std::stable_sort(out.begin(), out.end(), [](const Point& a, const Point& b) { return a.time < b.time; });
    out.erase(std::unique(out.begin(), out.end(), [](const Point& a, const Point& b) { return a.time == b.time; }),
              out.end());
    return out;
}

QString speed_description(double km_s) {
    if (!std::isfinite(km_s) || km_s < 0) return {};
    if (km_s < 400) return "Slow";
    if (km_s < 600) return "Moderate";
    if (km_s < 800) return "Fast";
    return "Very fast";
}

QString bz_description(double nt) {
    if (!std::isfinite(nt)) return {};
    if (nt > -3) return nt >= 0 ? "Northward" : "Near neutral";
    if (nt > -8) return "Weakly south";
    if (nt > -15) return "Southward";
    return "Strongly south";
}

} // namespace hamclock::solarwind
