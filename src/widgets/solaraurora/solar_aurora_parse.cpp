#include "solar_aurora_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <algorithm>

namespace hamclock::solaraurora {
namespace {
QDateTime utc(const QString& text) {
    auto t = QDateTime::fromString(text, Qt::ISODate);
    if (t.isValid()) t.setTimeZone(QTimeZone::utc());
    return t;
}
} // namespace

ColorScale aurora_scale() {
    return {"Aurora", "%", {{3, QColor("#2ecc71")}, {10, QColor("#a3e635")}, {20, QColor("#facc15")}, {40, QColor("#f97316")}, {70, QColor("#ef4444")}}};
}

Grid parse_ovation(const QByteArray& body) {
    Grid g;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) return g;
    const auto root = doc.object();
    g.observed = utc(root.value("Observation Time").toString());
    g.forecast = utc(root.value("Forecast Time").toString());
    const auto coords = root.value("coordinates").toArray();
    g.cells.assign(360 * 181, 0);
    int used = 0;
    for (const auto& v : coords) {
        const auto c = v.toArray();
        if (c.size() < 3 || !c[0].isDouble() || !c[1].isDouble() || !c[2].isDouble()) continue;
        const int lon = c[0].toInt(), lat = c[1].toInt(), p = c[2].toInt();
        if (lon < 0 || lon > 359 || lat < -90 || lat > 90) continue;
        g.cells[static_cast<size_t>((lat + 90) * 360 + lon)] = static_cast<unsigned char>(std::clamp(p, 0, 100));
        ++used;
    }
    g.valid = used > 0 && g.observed.isValid();
    return g;
}

int peak(const Grid& g, bool north) {
    if (!g.valid) return 0;
    int best = 0;
    for (int lat = north ? 0 : -90; lat <= (north ? 90 : -1); ++lat)
        for (int lon = 0; lon < 360; ++lon) best = std::max(best, g.at(lat, lon));
    return best;
}

QImage to_image(const Grid& g) {
    QImage img(360, 180, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    if (!g.valid) return img;
    for (int row = 0; row < 180; ++row) {
        const int lat = 90 - row;
        for (int x = 0; x < 360; ++x) {
            const int p = g.at(lat, x + 180);  // x = 0 is longitude -180, stored as 180
            if (p < 3) continue;
            QColor c = color_at(aurora_scale(), p);
            c.setAlphaF(std::min(1.0, (p - 2) / 28.0));
            img.setPixelColor(x, row, c);
        }
    }
    return img;
}

} // namespace hamclock::solaraurora
