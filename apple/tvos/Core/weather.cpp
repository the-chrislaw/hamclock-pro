#include "weather.hpp"
#include "json_mini.hpp"
#include <cmath>

namespace hcp::weather {
namespace {
bool number(const json::Value& o, const char* key, double lo, double hi, double& out) {
    const auto& v = o.at(key);
    if (!v.is_number()) return false;
    out = v.number;
    return std::isfinite(out) && out >= lo && out <= hi;
}
} // namespace

std::optional<Current> parse_current(std::string_view body) {
    const auto doc = json::parse(body);
    if (!doc || !doc->is_object()) return std::nullopt;
    const auto& cur = doc->at("current");
    Current c;
    double v = 0;
    if (!number(*doc, "latitude", -90, 90, c.latitude) || !number(*doc, "longitude", -180, 180, c.longitude))
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
    // Hourly series (past_hours=3&forecast_hours=1): oldest first, so last - first is the 3 h change.
    const auto& series = doc->at("hourly").at("pressure_msl");
    if (series.is_array() && series.array.size() >= 2) {
        const auto& first = series.array.front();
        const auto& last = series.array.back();
        if (first.is_number() && last.is_number() && std::isfinite(first.number) && first.number >= 800 &&
            first.number <= 1100 && last.number >= 800 && last.number <= 1100)
            c.pressure_change_hpa = last.number - first.number;
    }
    const auto& day = cur.at("is_day");
    c.is_day = !day.is_number() || day.number != 0;
    return c;
}

std::string wmo_description(int code) {
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

std::string compass_point(int degrees) {
    static const char* const rose[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                       "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    const int d = ((degrees % 360) + 360) % 360;
    return rose[static_cast<int>((d + 11.25) / 22.5) % 16];
}

std::string pressure_trend_word(double change_hpa) {
    if (change_hpa > 1.0) return "Rising";
    if (change_hpa < -1.0) return "Falling";
    return "Steady";
}

std::optional<double> dew_point_c(double t, int rh) {
    if (rh <= 0 || rh > 100 || !(t >= -45 && t <= 60)) return std::nullopt;
    constexpr double a = 17.62, b = 243.12; // Magnus coefficients (Sonntag 1990)
    const double g = std::log(rh / 100.0) + a * t / (b + t);
    return b * g / (a - g);
}

} // namespace hcp::weather
