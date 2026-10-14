#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <optional>

namespace hamclock::weather {

struct Current {
    double latitude = 0, longitude = 0;  // grid point the service answered for
    double temperature_c = 0;
    int humidity_pct = 0;
    double wind_kmh = 0;
    int wind_dir_deg = 0;
    int weather_code = 0;                // WMO code
    bool is_day = true;
    std::optional<double> pressure_hpa;  // mean sea level; absent if the service omits it
    std::optional<int> cloud_pct;
    std::optional<int> utc_offset_s;     // local zone of the point (timezone=auto); absent if omitted
    std::optional<double> pressure_change_hpa;  // last hourly value minus the one 3 h earlier
};

// Open-Meteo `current` block (units: °C, %, km/h, degrees). Anything missing,
// non-numeric or out of physical range rejects the whole document.
std::optional<Current> parse_current(const QByteArray& body);
// "Rising" / "Falling" / "Steady" for a 3-hour change (steady within ±1 hPa).
QString pressure_trend_word(double change_hpa);
QString wmo_description(int code);        // "Clear sky", "Light rain", …
QString compass_point(int degrees);       // 16-point rose, direction wind comes FROM
double celsius_to_fahrenheit(double c);
// "14:32 (UTC-7)" for a UTC instant and offset in seconds.
QString local_time_text(const QDateTime& utc, int offset_s);
// Request URL for a point; coordinates rounded to 2 decimals (~1 km).
QString request_url(double latitude, double longitude);

} // namespace hamclock::weather
