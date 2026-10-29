#pragma once
// Qt-free port of src/widgets/solarwind/solar_wind_parse (NOAA SWPC rtsw_*_1m.json).
// Same rules as the Qt parser, which stays the behavioural oracle in tests.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::solarwind {

struct Point {
    int64_t time_ms = 0; // UTC, one-minute cadence
    double value = 0;
};

// Only rows with "active": true; null / non-numeric / -9999 fill values skipped.
// Oldest first, at most one point per timestamp.
std::vector<Point> series(std::string_view body, const std::string& field);
// Same rows; nullopt when the body is not a JSON array.
std::optional<std::vector<Point>> checked_series(std::string_view body, const std::string& field);
std::optional<Point> latest(std::string_view body, const std::string& field);

inline constexpr char kSpeed[] = "proton_speed";     // km/s (rtsw_wind_1m)
inline constexpr char kDensity[] = "proton_density"; // p/cm^3 (rtsw_wind_1m)
inline constexpr char kBt[] = "bt";                  // nT (rtsw_mag_1m)
inline constexpr char kBz[] = "bz_gsm";              // nT, GSM (rtsw_mag_1m)

std::string speed_description(double km_s); // Slow / Moderate / Fast / Very fast; "" invalid
std::string bz_description(double nt);      // Northward / Near neutral / Weakly south / ...

} // namespace hcp::solarwind
