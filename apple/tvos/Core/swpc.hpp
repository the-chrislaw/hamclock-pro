#pragma once
// Qt-free port of src/widgets/spaceweather/swpc_parse (NOAA SWPC JSON products).
// Same rules as the Qt parser, which stays the behavioural oracle in tests.
// Pure functions over the raw body: no network, no clock. Times are UTC epoch ms.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::swpc {

struct Reading {
    double value = 0;
    int64_t time_ms = 0; // UTC observation time from the product
};

// ISO-8601 "YYYY-MM-DDTHH:MM:SS[.fff][Z]" (UTC assumed) -> epoch ms.
std::optional<int64_t> parse_iso_utc_ms(std::string_view text);

std::optional<Reading> latest_kp(std::string_view body);          // Kp in [0, 9]
std::optional<Reading> latest_solar_flux(std::string_view body);  // sfu, (0, 1000]
std::optional<Reading> latest_xray_flux(std::string_view body);   // 0.1-0.8nm, W/m^2
std::vector<Reading> xray_series(std::string_view body, int bucket_minutes);
// Every valid row, oldest first (stable for equal times); nullopt when the body is not a JSON array.
std::optional<std::vector<Reading>> kp_history(std::string_view body);
std::optional<std::vector<Reading>> xray_history(std::string_view body); // 0.1-0.8nm only, unbucketed
double xray_plot_fraction(double flux_w_m2);
std::string xray_class(double flux_w_m2);      // 3.2e-6 -> "C3.2"; "" when invalid
std::string kp_description(double kp);         // NOAA G-scale wording

} // namespace hcp::swpc
