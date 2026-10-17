// Qt-free Open-Meteo parser; mirrors src/widgets/weather/tests (the oracle) on the same fixture.
#include "../hcpcore.h"
#include "../weather.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
std::string fixture(const char* name) {
    std::ifstream in(std::string(HCP_FIXTURE_DIR) + "/" + name, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
} // namespace

int main() {
    using namespace hcp;
    const auto body = fixture("forecast");
    check(!body.empty(), "fixture present");
    const auto c = weather::parse_current(body);
    check(c && c->temperature_c == 15.7 && c->humidity_pct == 61, "temperature and humidity");
    check(c && c->wind_kmh == 7.9 && c->wind_dir_deg == 344 && c->weather_code == 0 && c->is_day, "wind, code, day");
    check(c && !c->pressure_hpa && !c->pressure_change_hpa, "optional fields absent in older response");

    const auto full = weather::parse_current(
        R"({"latitude":1,"longitude":1,"current":{"temperature_2m":10,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3,"is_day":0,"pressure_msl":1015.4},
        "hourly":{"pressure_msl":[1012.0,1013.0,1014.0,1015.4]}})");
    check(full && full->pressure_hpa && *full->pressure_hpa == 1015.4 && !full->is_day, "pressure and night");
    check(full && full->pressure_change_hpa && std::fabs(*full->pressure_change_hpa - 3.4) < 1e-9, "3 h pressure change");
    check(weather::pressure_trend_word(3.4) == "Rising" && weather::pressure_trend_word(-1.5) == "Falling" &&
          weather::pressure_trend_word(1.0) == "Steady", "trend words");

    check(!weather::parse_current("") && !weather::parse_current("[]") && !weather::parse_current("{}"), "non-forecast rejected");
    check(!weather::parse_current(
              R"({"latitude":1,"longitude":1,"current":{"temperature_2m":200,"relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3}})"),
          "out-of-range temperature rejects");
    check(!weather::parse_current(
              R"({"latitude":1,"longitude":1,"current":{"temperature_2m":"x","relative_humidity_2m":50,"wind_speed_10m":1,"wind_direction_10m":1,"weather_code":3}})"),
          "non-numeric rejects");

    const auto dew = weather::dew_point_c(20, 50);
    check(dew && std::fabs(*dew - 9.27) < 0.1, "dew point at 20 C / 50 %");
    check(weather::dew_point_c(20, 100) && std::fabs(*weather::dew_point_c(20, 100) - 20) < 0.01, "dew point at 100 % RH");
    check(!weather::dew_point_c(20, 0) && !weather::dew_point_c(80, 50), "dew point rejects bad input");
    check(weather::compass_point(344) == "NNW" && weather::compass_point(0) == "N" && weather::compass_point(360) == "N" &&
          weather::compass_point(349) == "N" && weather::compass_point(90) == "E", "compass rose edges");
    check(weather::wmo_description(0) == "Clear sky" && weather::wmo_description(95) == "Thunderstorm" &&
          weather::wmo_description(42) == "Unknown", "WMO descriptions");

    HcpWeather w;
    check(hcp_weather_parse(body.data(), body.size(), &w) == 1 && w.temperature_c == 15.7 && w.humidity_pct == 61 &&
          w.has_pressure == 0 && std::string(w.description) == "Clear sky" && std::string(w.wind_from) == "NNW" &&
          w.has_dew_point == 1 && std::fabs(w.dew_point_c - 8.3) < 0.2, "C ABI parse");
    check(hcp_weather_parse("garbage", 7, &w) == 0 && hcp_weather_parse(nullptr, 0, &w) == 0 &&
          hcp_weather_parse(body.data(), body.size(), nullptr) == 0, "C ABI bad input");
    return failures ? 1 : 0;
}
