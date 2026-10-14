#include "location_tools.hpp"

#include "../domain/locator_input.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrlQuery>
#include <cmath>
#include <limits>

namespace hamclock {

double LocationTools::parseCoordinate(const QString& text, bool latitude) const {
    const double bad = std::numeric_limits<double>::quiet_NaN();
    QString s = text.trimmed().toUpper();
    if (s.isEmpty()) return bad;
    const QString pos = latitude ? "N" : "E", neg = latitude ? "S" : "W";
    double sign = 1;
    bool hemisphere = false;
    auto take = [&](const QString& letter, double value, bool at_start) {
        if (hemisphere) return;
        if (at_start ? s.startsWith(letter) : s.endsWith(letter)) {
            s = at_start ? s.mid(1) : s.chopped(1);
            sign = value;
            hemisphere = true;
        }
    };
    for (bool start : {true, false}) { take(pos, 1, start); take(neg, -1, start); }
    s = s.trimmed();
    if (s.startsWith('-') || s.startsWith('+')) {
        if (hemisphere) return bad;  // "-40 S" is ambiguous
        if (s[0] == '-') sign = -1;
        s = s.mid(1);
    }
    // degrees[ minutes[ seconds]] separated by : space ° ' " (no letters left)
    static const QRegularExpression sep("[:\\s°'\"′″]+");
    const QStringList parts = s.split(sep, Qt::SkipEmptyParts);
    if (parts.isEmpty() || parts.size() > 3) return bad;
    double total = 0, scale = 1;
    for (int i = 0; i < parts.size(); ++i) {
        static const QRegularExpression number("^\\d+(\\.\\d+)?$");
        if (!number.match(parts[i]).hasMatch()) return bad;
        const double v = parts[i].toDouble();
        if (i > 0 && (v >= 60 || (i < parts.size() - 1 && v != std::floor(v)))) return bad;
        total += v / scale;
        scale *= 60;
    }
    const double value = sign * total;
    return std::fabs(value) <= (latitude ? 90.0 : 180.0) ? value : bad;
}

QVariantMap LocationTools::fromGrid(const QString& grid) const {
    const QString g = grid.trimmed();
    if (g.size() != 2 && g.size() != 4 && g.size() != 6) return {};
    try {
        const Location corner = location_from_maidenhead(g.toStdString());
        // Cell size: field 10x20 deg, square 1x2, subsquare 2.5'x5'.
        const double lat_h = g.size() == 2 ? 10.0 : g.size() == 4 ? 1.0 : 1.0 / 24;
        const double lon_w = 2 * lat_h;
        return {{"latitude", corner.latitude + lat_h / 2}, {"longitude", corner.longitude + lon_w / 2}};
    } catch (const std::exception&) {
        return {};
    }
}

QString LocationTools::gridFor(double latitude, double longitude) const {
    if (!(std::fabs(latitude) <= 90.0) || !(std::fabs(longitude) <= 180.0)) return {};
    double lon = std::min(longitude + 180.0, 359.999999), lat = std::min(latitude + 90.0, 179.999999);
    QString out;
    out += QChar('A' + int(lon / 20));  lon = std::fmod(lon, 20);
    out += QChar('A' + int(lat / 10));  lat = std::fmod(lat, 10);
    out += QChar('0' + int(lon / 2));   lon = std::fmod(lon, 2);
    out += QChar('0' + int(lat));       lat = std::fmod(lat, 1);
    out += QChar('a' + int(lon * 12));
    out += QChar('a' + int(lat * 24));
    return out;
}

QVariantList LocationTools::parseSearchReply(const QByteArray& json) const {
    QVariantList out;
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isArray()) return out;
    for (const QJsonValue& v : doc.array()) {
        const QJsonObject o = v.toObject();
        bool lat_ok = false, lon_ok = false;
        const double lat = o.value("lat").toString().toDouble(&lat_ok);
        const double lon = o.value("lon").toString().toDouble(&lon_ok);
        if (!lat_ok || !lon_ok || std::fabs(lat) > 90 || std::fabs(lon) > 180) continue;
        out.push_back(QVariantMap{{"name", o.value("display_name").toString()}, {"latitude", lat}, {"longitude", lon}});
    }
    return out;
}

QUrl LocationTools::searchUrl(const QString& query) {
    QUrl url("https://nominatim.openstreetmap.org/search");
    QUrlQuery q;
    q.addQueryItem("q", query.trimmed());
    q.addQueryItem("format", "jsonv2");
    q.addQueryItem("limit", "5");
    url.setQuery(q);
    return url;
}

void LocationTools::search(const QString& query) {
    if (query.trimmed().size() < 3) { emit searchFinished({}, "Type at least 3 characters to search."); return; }
    if (!transport_) { emit searchFinished({}, "Search is not available."); return; }
    if (pending_) return;
    if (last_request_.isValid() && last_request_.elapsed() < 1000) {
        emit searchFinished({}, "Please wait a second and try again.");
        return;
    }
    last_request_.start();
    pending_ = true;
    transport_(searchUrl(query), [this](FetchResult r) {
        pending_ = false;
        if (!r.error.isEmpty() || r.http_status != 200) {
            emit searchFinished({}, "Search failed: " + (r.error.isEmpty() ? QString("HTTP %1").arg(r.http_status) : r.error));
            return;
        }
        const auto hits = parseSearchReply(r.body);
        emit searchFinished(hits, hits.isEmpty() ? QString("No match.") : QString());
    });
}

} // namespace hamclock
