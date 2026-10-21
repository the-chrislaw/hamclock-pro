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
    std::optional<double> high_c, low_c;        // today's forecast extremes (local day); absent if omitted
    std::optional<double> tomorrow_high_c, tomorrow_low_c;  // tomorrow's forecast extremes; absent if omitted
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
// Dew point (Magnus formula, °C) from air temperature and relative humidity; computed locally, no extra request.
// Absent for humidity <= 0 or outside -45..60 °C where the formula is not meaningful.
std::optional<double> dew_point_c(double temperature_c, int humidity_pct);
// "14:32 (UTC-7)" for a UTC instant and offset in seconds.
QString local_time_text(const QDateTime& utc, int offset_s);
// Place label from a Nominatim `reverse` reply (addressdetails=1): the city, town
// or village (else county/state) plus the ISO country code, e.g. "San Francisco, US".
// Empty for an error reply (open sea) or an unusable one.
QString parse_place(const QByteArray& body);
// Nominatim reverse request (zoom 10 = city level); coordinates rounded to 2 decimals.
QString reverse_url(double latitude, double longitude);
// Windy map page centred on a point (https://www.windy.com/?lat,lon,zoom); a plain link the user opens, never fetched by the app.
QString windy_url(double latitude, double longitude);

// NOAA marine radiofax (WEFAX) schedules and charts; a plain link the user opens, never fetched by the app.
QString wefax_url();
// Request URL for a point; coordinates rounded to 2 decimals (~1 km).
QString request_url(double latitude, double longitude);

} // namespace hamclock::weather
