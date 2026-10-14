#include "solar_drap_parse.hpp"
#include <QTimeZone>
#include <algorithm>

namespace hamclock::solardrap {

ColorScale drap_scale() {
    return {"D-RAP", "MHz", {{1, QColor("#38bdf8")}, {5, QColor("#2ecc71")}, {10, QColor("#facc15")}, {15, QColor("#f97316")}, {25, QColor("#ef4444")}}};
}

Grid parse_drap(const QByteArray& body) {
    Grid g;
    std::vector<float> values;
    int rows = 0;
    bool bad = false;
    for (const auto& raw : body.split('\n')) {
        const QString line = QString::fromLatin1(raw).trimmed();
        if (line.startsWith('#')) {
            const auto colon = line.indexOf(':');
            if (colon < 0) continue;
            const QString key = line.left(colon), val = line.mid(colon + 1).trimmed();
            if (key.contains("Valid At")) {
                auto t = QDateTime::fromString(val.left(16), "yyyy-MM-dd HH:mm");
                if (t.isValid()) { t.setTimeZone(QTimeZone::utc()); g.valid_at = t; }
            } else if (key.contains("X-RAY Message")) g.xray = val;
            else if (key.contains("Proton Message")) g.proton = val;
            continue;
        }
        const auto bar = line.indexOf('|');
        if (bar < 0) continue;
        const auto parts = line.mid(bar + 1).split(' ', Qt::SkipEmptyParts);
        if (parts.size() != kCols) { bad = true; break; }
        for (const auto& p : parts) {
            bool ok = false;
            const float v = p.toFloat(&ok);
            if (!ok || v < 0) { bad = true; break; }
            values.push_back(v);
        }
        if (bad) break;
        ++rows;
    }
    if (bad || rows != kRows || !g.valid_at.isValid()) return Grid{};
    g.mhz = std::move(values);
    g.valid = true;
    return g;
}

double peak(const Grid& g) {
    if (!g.valid) return 0;
    return *std::max_element(g.mhz.begin(), g.mhz.end());
}

QImage to_image(const Grid& g) {
    QImage img(360, 180, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    if (!g.valid) return img;
    for (int y = 0; y < 180; ++y) {
        for (int x = 0; x < 360; ++x) {
            const float v = g.mhz[static_cast<size_t>((y / 2) * kCols + x / 4)];
            if (v < 1.0f) continue;
            QColor c = color_at(drap_scale(), v);
            c.setAlphaF(std::min(1.0, 0.35 + v / 12.0));
            img.setPixelColor(x, y, c);
        }
    }
    return img;
}

} // namespace hamclock::solardrap
