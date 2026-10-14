#include "weather_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <cmath>

namespace hamclock::weather {
namespace {
bool number(const QJsonObject& o, const char* key, double lo, double hi, double& out) {
    const QJsonValue v = o.value(key);
    if (!v.isDouble()) return false;
    out = v.toDouble();
    return std::isfinite(out) && out >= lo && out <= hi;
}
} // namespace

std::optional<Current> parse_current(const QByteArray& body) {
    const auto root = QJsonDocument::fromJson(body).object();
    const auto cur = root.value("current").toObject();
    Current c;
    double v = 0;
    if (!number(root, "latitude", -90, 90, c.latitude) || !number(root, "longitude", -180, 180, c.longitude))
        return std::nullopt;
    if (!number(cur, "temperature_2m", -100, 70, c.temperature_c)) return std::nullopt;
    if (!number(cur, "relative_humidity_2m", 0, 100, v)) return std::nullopt;
    c.humidity_pct = static_cast<int>(std::lround(v));
    if (!number(cur, "wind_speed_10m", 0, 500, c.wind_kmh)) return std::nullopt;
    if (!number(cur, "wind_direction_10m", 0, 360, v)) return std::nullopt;
    c.wind_dir_deg = static_cast<int>(std::lround(v)) % 360;
    if (!number(cur, "weather_code", 0, 99, v)) return std::nullopt;
    c.weather_code = static_cast<int>(v);
    if (number(cur, "pressure_msl", 800, 1100, v)) c.pressure_hpa = v;
    if (number(cur, "cloud_cover", 0, 100, v)) c.cloud_pct = static_cast<int>(std::lround(v));
    // Hourly series (past_hours=3&forecast_hours=1): oldest first, so last - first is the 3 h change.
    const auto series = root.value("hourly").toObject().value("pressure_msl").toArray();
    if (series.size() >= 2) {
        const QJsonValue first = series.first(), last = series.last();
        if (first.isDouble() && last.isDouble() && std::isfinite(first.toDouble()) &&
            first.toDouble() >= 800 && first.toDouble() <= 1100 && last.toDouble() >= 800 && last.toDouble() <= 1100)
            c.pressure_change_hpa = last.toDouble() - first.toDouble();
    }
    c.is_day = cur.value("is_day").toDouble(1) != 0;
    return c;
}

QString pressure_trend_word(double change_hpa) {
    if (change_hpa > 1.0) return "Rising";
    if (change_hpa < -1.0) return "Falling";
    return "Steady";
}

QString wmo_description(int code) {
    switch (code) {
    case 0: return "Clear sky";
    case 1: return "Mainly clear";
    case 2: return "Partly cloudy";
    case 3: return "Overcast";
    case 45: case 48: return "Fog";
    case 51: case 53: case 55: return "Drizzle";
    case 56: case 57: return "Freezing drizzle";
    case 61: return "Light rain";
    case 63: return "Rain";
    case 65: return "Heavy rain";
    case 66: case 67: return "Freezing rain";
    case 71: return "Light snow";
    case 73: return "Snow";
    case 75: return "Heavy snow";
    case 77: return "Snow grains";
    case 80: case 81: case 82: return "Rain showers";
    case 85: case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96: case 99: return "Thunderstorm with hail";
    default: return "Unknown";
    }
}

QString compass_point(int degrees) {
    static const char* const rose[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                       "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    const int d = ((degrees % 360) + 360) % 360;
    return rose[static_cast<int>((d + 11.25) / 22.5) % 16];
}

double celsius_to_fahrenheit(double c) { return c * 9.0 / 5.0 + 32.0; }

QString request_url(double latitude, double longitude) {
    return QString("https://api.open-meteo.com/v1/forecast?latitude=%1&longitude=%2"
                   "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,wind_direction_10m,"
                   "weather_code,is_day,pressure_msl,cloud_cover&hourly=pressure_msl&past_hours=3&forecast_hours=1&timezone=UTC")
        .arg(latitude, 0, 'f', 2).arg(longitude, 0, 'f', 2);
}

} // namespace hamclock::weather
