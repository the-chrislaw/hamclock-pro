#pragma once
// Qt-free port of src/widgets/weather/weather_parse (Open-Meteo `current` block).
// Same ranges and rules as the Qt parser, which stays the behavioural oracle in tests.
#include <optional>
#include <string>
#include <string_view>

namespace hcp::weather {

struct Current {
    double latitude = 0, longitude = 0; // grid point the service answered for
    double temperature_c = 0;
    int humidity_pct = 0;
    double wind_kmh = 0;
    int wind_dir_deg = 0;
    int weather_code = 0; // WMO code
    bool is_day = true;
    std::optional<double> pressure_hpa;
    std::optional<double> pressure_change_hpa; // last hourly value minus the one 3 h earlier
};

// Anything missing, non-numeric or out of physical range rejects the whole document.
std::optional<Current> parse_current(std::string_view body);
std::string wmo_description(int code); // "Clear sky", "Light rain", ...
std::string compass_point(int degrees); // 16-point rose, direction wind comes FROM
std::string pressure_trend_word(double change_hpa); // Rising / Falling / Steady (within +-1 hPa)
// Magnus dew point; nullopt for humidity <= 0 or outside -45..60 C.
std::optional<double> dew_point_c(double temperature_c, int humidity_pct);

} // namespace hcp::weather
