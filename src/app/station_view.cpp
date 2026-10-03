#include "station_view.hpp"

#include <cmath>
#include <stdexcept>

namespace hamclock {
namespace {
std::string uppercase(std::string value) {
    for (char& c : value)
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    return value;
}
void validate_view_settings(const StationSettings& settings) {
    if (!valid_location(settings.de) || !valid_location(settings.dx))
        throw std::invalid_argument("Invalid station coordinates");
    for (const double coordinate : {settings.de.latitude, settings.de.longitude,
                                    settings.dx.latitude, settings.dx.longitude})
        if (std::fpclassify(coordinate) == FP_SUBNORMAL)
            throw std::invalid_argument("Subnormal coordinates are not supported in settings");
    if (settings.callsign.empty() || settings.callsign.size() > 16)
        throw std::invalid_argument("Callsign must contain 1 to 16 ASCII characters");
    for (const unsigned char c : settings.callsign)
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '/' || c == '-'))
            throw std::invalid_argument("Invalid callsign character");
    if (settings.appearance != "system" && settings.appearance != "light" &&
        settings.appearance != "dark")
        throw std::invalid_argument("Invalid appearance");
}
} // namespace

StationView make_station_view(const StationSettings& settings) {
    validate_view_settings(settings);
    const auto path = geometry(settings.de, settings.dx);
    std::string bearing = "BEARING UNDEFINED";
    if (path.status == BearingStatus::Defined) {
        // Rounded 360 represents north and is displayed as zero degrees.
        const auto rounded = std::llround(path.short_bearing_deg) % 360;
        bearing = std::to_string(rounded) + " DEG / TRUE";
    }
    return {uppercase(settings.callsign), uppercase(maidenhead(settings.de)),
            uppercase(maidenhead(settings.dx)),
            std::to_string(std::llround(path.distance_km)) + " KM", bearing,
            settings.de, settings.dx};
}
} // namespace hamclock
