#include "time_zone_search.hpp"

#include <QDateTime>
#include <QStringList>
#include <QTimeZone>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace hamclock {
namespace {

QDateTime when(qint64 at_utc_secs) {
    return at_utc_secs < 0 ? QDateTime::currentDateTimeUtc() : QDateTime::fromSecsSinceEpoch(at_utc_secs, Qt::UTC);
}

QString city_of(const QString& id) { return id.mid(id.lastIndexOf('/') + 1).replace('_', ' '); }

QString offset_text(int seconds) {
    const QChar sign = seconds < 0 ? QChar(0x2212) : QChar('+');
    const int abs_minutes = std::abs(seconds) / 60;
    QString text = QStringLiteral("UTC") + sign + QString::number(abs_minutes / 60);
    if (abs_minutes % 60) text += QString(":%1").arg(abs_minutes % 60, 2, 10, QChar('0'));
    return text;
}

QString label(const QString& id, const QDateTime& at) {
    const QTimeZone tz(id.toUtf8());
    return city_of(id) + " (" + offset_text(tz.offsetFromUtc(at)) + ")";
}

struct City { const char* zone; double lat; double lon; };
const City kCities[] = {
    {"America/New_York", 40.7, -74.0},
    {"America/Chicago", 41.9, -87.6},
    {"America/Denver", 39.7, -105.0},
    {"America/Phoenix", 33.4, -112.1},
    {"America/Los_Angeles", 34.1, -118.2},
    {"America/Los_Angeles", 47.6, -122.3},
    {"America/Anchorage", 61.2, -149.9},
    {"Pacific/Honolulu", 21.3, -157.9},
    {"America/Halifax", 44.6, -63.6},
    {"America/St_Johns", 47.6, -52.7},
    {"America/Toronto", 43.7, -79.4},
    {"America/Winnipeg", 49.9, -97.1},
    {"America/Edmonton", 53.5, -113.5},
    {"America/Vancouver", 49.3, -123.1},
    {"America/Regina", 50.4, -104.6},
    {"America/Mexico_City", 19.4, -99.1},
    {"America/Havana", 23.1, -82.4},
    {"America/Panama", 9.0, -79.5},
    {"America/Bogota", 4.7, -74.1},
    {"America/Lima", -12.0, -77.0},
    {"America/Caracas", 10.5, -66.9},
    {"America/La_Paz", -16.5, -68.1},
    {"America/Santiago", -33.4, -70.7},
    {"America/Argentina/Buenos_Aires", -34.6, -58.4},
    {"America/Sao_Paulo", -23.6, -46.6},
    {"America/Manaus", -3.1, -60.0},
    {"America/Puerto_Rico", 18.5, -66.1},
    {"America/Guatemala", 14.6, -90.5},
    {"Atlantic/Reykjavik", 64.1, -21.9},
    {"Atlantic/Azores", 37.7, -25.7},
    {"Atlantic/Canary", 28.1, -15.4},
    {"Europe/London", 51.5, -0.1},
    {"Europe/Dublin", 53.3, -6.3},
    {"Europe/Lisbon", 38.7, -9.1},
    {"Europe/Madrid", 40.4, -3.7},
    {"Europe/Paris", 48.9, 2.4},
    {"Europe/Brussels", 50.8, 4.4},
    {"Europe/Amsterdam", 52.4, 4.9},
    {"Europe/Berlin", 52.5, 13.4},
    {"Europe/Zurich", 47.4, 8.5},
    {"Europe/Rome", 41.9, 12.5},
    {"Europe/Vienna", 48.2, 16.4},
    {"Europe/Prague", 50.1, 14.4},
    {"Europe/Warsaw", 52.2, 21.0},
    {"Europe/Copenhagen", 55.7, 12.6},
    {"Europe/Oslo", 59.9, 10.8},
    {"Europe/Stockholm", 59.3, 18.1},
    {"Europe/Helsinki", 60.2, 24.9},
    {"Europe/Tallinn", 59.4, 24.7},
    {"Europe/Athens", 38.0, 23.7},
    {"Europe/Bucharest", 44.4, 26.1},
    {"Europe/Kyiv", 50.5, 30.5},
    {"Europe/Istanbul", 41.0, 29.0},
    {"Europe/Moscow", 55.8, 37.6},
    {"Europe/Kaliningrad", 54.7, 20.5},
    {"Europe/Belgrade", 44.8, 20.5},
    {"Africa/Casablanca", 33.6, -7.6},
    {"Africa/Algiers", 36.8, 3.1},
    {"Africa/Tunis", 36.8, 10.2},
    {"Africa/Cairo", 30.0, 31.2},
    {"Africa/Lagos", 6.5, 3.4},
    {"Africa/Accra", 5.6, -0.2},
    {"Africa/Nairobi", -1.3, 36.8},
    {"Africa/Addis_Ababa", 9.0, 38.7},
    {"Africa/Johannesburg", -26.2, 28.0},
    {"Africa/Kinshasa", -4.3, 15.3},
    {"Africa/Maputo", -25.9, 32.6},
    {"Asia/Jerusalem", 31.8, 35.2},
    {"Asia/Riyadh", 24.7, 46.7},
    {"Asia/Dubai", 25.3, 55.3},
    {"Asia/Tehran", 35.7, 51.4},
    {"Asia/Baghdad", 33.3, 44.4},
    {"Asia/Karachi", 24.9, 67.0},
    {"Asia/Tashkent", 41.3, 69.2},
    {"Asia/Almaty", 43.2, 76.9},
    {"Asia/Kolkata", 28.6, 77.2},
    {"Asia/Kolkata", 19.1, 72.9},
    {"Asia/Colombo", 6.9, 79.9},
    {"Asia/Kathmandu", 27.7, 85.3},
    {"Asia/Dhaka", 23.8, 90.4},
    {"Asia/Yangon", 16.8, 96.2},
    {"Asia/Bangkok", 13.8, 100.5},
    {"Asia/Ho_Chi_Minh", 10.8, 106.7},
    {"Asia/Jakarta", -6.2, 106.8},
    {"Asia/Makassar", -5.1, 119.4},
    {"Asia/Jayapura", -2.5, 140.7},
    {"Asia/Singapore", 1.35, 103.8},
    {"Asia/Kuala_Lumpur", 3.1, 101.7},
    {"Asia/Manila", 14.6, 121.0},
    {"Asia/Hong_Kong", 22.3, 114.2},
    {"Asia/Shanghai", 31.2, 121.5},
    {"Asia/Shanghai", 39.9, 116.4},
    {"Asia/Urumqi", 43.8, 87.6},
    {"Asia/Taipei", 25.0, 121.5},
    {"Asia/Seoul", 37.6, 127.0},
    {"Asia/Tokyo", 35.7, 139.7},
    {"Asia/Novosibirsk", 55.0, 82.9},
    {"Asia/Yekaterinburg", 56.8, 60.6},
    {"Asia/Irkutsk", 52.3, 104.3},
    {"Asia/Vladivostok", 43.1, 131.9},
    {"Asia/Kamchatka", 53.0, 158.7},
    {"Asia/Ulaanbaatar", 47.9, 106.9},
    {"Australia/Perth", -31.95, 115.9},
    {"Australia/Darwin", -12.5, 130.8},
    {"Australia/Adelaide", -34.9, 138.6},
    {"Australia/Brisbane", -27.5, 153.0},
    {"Australia/Sydney", -33.9, 151.2},
    {"Australia/Melbourne", -37.8, 145.0},
    {"Australia/Hobart", -42.9, 147.3},
    {"Pacific/Auckland", -36.85, 174.8},
    {"Pacific/Fiji", -18.1, 178.4},
    {"Pacific/Port_Moresby", -9.5, 147.2},
    {"Pacific/Guam", 13.5, 144.8},
    {"Pacific/Pago_Pago", -14.3, -170.7},
    {"Pacific/Tahiti", -17.5, -149.6},
    {"Indian/Mauritius", -20.2, 57.5},
};

double distance_km(double lat1, double lon1, double lat2, double lon2) {
    constexpr double kRad = 3.14159265358979323846 / 180.0;
    const double dlat = (lat2 - lat1) * kRad, dlon = (lon2 - lon1) * kRad;
    const double a = std::sin(dlat / 2) * std::sin(dlat / 2) +
                     std::cos(lat1 * kRad) * std::cos(lat2 * kRad) * std::sin(dlon / 2) * std::sin(dlon / 2);
    return 12742.0 * std::asin(std::sqrt(std::min(1.0, a)));
}

} // namespace

QString TimeZoneSearch::suggestZone(double latitude, double longitude) const {
    if (!(latitude >= -90 && latitude <= 90 && longitude >= -180 && longitude <= 180)) return {};
    const City* best = nullptr;
    double best_km = 1e9;
    for (const auto& city : kCities) {
        const double km = distance_km(latitude, longitude, city.lat, city.lon);
        if (km < best_km) { best_km = km; best = &city; }
    }
    if (!best || best_km > 1500.0) return {};
    const QString id = QString::fromLatin1(best->zone);
    return QTimeZone::isTimeZoneIdAvailable(id.toUtf8()) ? id : QString();
}

QVariantList TimeZoneSearch::search(const QString& query, int limit, qint64 at_utc_secs) const {
    const auto words = query.toLower().replace('_', ' ').split(' ', Qt::SkipEmptyParts);
    auto ids = QTimeZone::availableTimeZoneIds();
    std::sort(ids.begin(), ids.end());
    const auto at = when(at_utc_secs);
    QVariantList out;
    for (const auto& raw : ids) {
        const QString id = QString::fromUtf8(raw);
        if (!id.contains('/')) continue;  // skip legacy aliases such as "EST5EDT"
        const QString haystack = id.toLower().replace('_', ' ');
        if (!std::all_of(words.begin(), words.end(), [&](const QString& w) { return haystack.contains(w); })) continue;
        out.push_back(QVariantMap{{"id", id}, {"label", label(id, at)}, {"region", id.left(id.indexOf('/'))}});
        if (out.size() >= limit) break;
    }
    return out;
}

QString TimeZoneSearch::labelFor(const QString& id, qint64 at_utc_secs) const {
    if (!QTimeZone::isTimeZoneIdAvailable(id.toUtf8())) return {};
    return label(id, when(at_utc_secs));
}

QString TimeZoneSearch::systemZone() const {
    const QString id = QString::fromUtf8(QTimeZone::systemTimeZoneId());
    return QTimeZone::isTimeZoneIdAvailable(id.toUtf8()) ? id : QString();
}

QString TimeZoneSearch::stationZone(const QString& saved, double latitude, double longitude) const {
    const QString chosen = saved.trimmed();
    if (!chosen.isEmpty()) return chosen;
    const QString suggested = suggestZone(latitude, longitude);
    return suggested.isEmpty() ? systemZone() : suggested;
}

} // namespace hamclock
