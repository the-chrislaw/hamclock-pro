#include "geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace hamclock {
namespace {
constexpr double pi = 3.1415926535897932384626433832795;
constexpr double radius_km = 6371.0088;
constexpr double degenerate_tolerance = 1e-12;
double radians(double degrees) { return degrees * pi / 180.0; }
double longitude(double value) { return value == 180.0 ? -180.0 : value; }
double azimuth(double radians_value) {
    double degrees = std::fmod(radians_value * 180.0 / pi + 360.0, 360.0);
    return degrees == 0.0 ? 0.0 : degrees;
}
// Each coordinate axis has 4320 six-character cells. Boundaries use the
// same representable rational reconstruction as the locator-input decoder.
// Compare coordinates directly rather than adding an offset/scaling, which
// can erase an ULP and put a canonical corner in its preceding cell.
unsigned locator_axis(double coordinate,double divisor,double offset) {
    unsigned low=0,high=4320;
    while(low+1<high) {
        const unsigned middle=low+(high-low)/2;
        const double boundary=static_cast<double>(middle)/divisor-offset;
        if(coordinate>=boundary)low=middle;
        else high=middle;
    }
    return low;
}
void require_valid(Location location) {
    if (!valid_location(location))
        throw std::invalid_argument("Latitude/longitude must be finite and within geographic bounds");
}
} // namespace

bool valid_location(Location location) noexcept {
    return std::isfinite(location.latitude) && std::isfinite(location.longitude)
        && location.latitude >= -90.0 && location.latitude <= 90.0
        && location.longitude >= -180.0 && location.longitude <= 180.0;
}

GeometryResult geometry(Location de, Location dx) {
    require_valid(de);
    require_valid(dx);
    const double p1 = radians(de.latitude), p2 = radians(dx.latitude);
    const double dl = std::remainder(radians(longitude(dx.longitude) - longitude(de.longitude)), 2.0 * pi);
    const double dp = p2 - p1;
    const double s = std::sin(dp / 2.0), t = std::sin(dl / 2.0);
    const double a = std::clamp(s * s + std::cos(p1) * std::cos(p2) * t * t, 0.0, 1.0);
    double angle = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    const double east = std::cos(p2) * std::sin(dl);
    const double north = std::cos(p1) * std::sin(p2) - std::sin(p1) * std::cos(p2) * std::cos(dl);
    const double sine_angle = std::hypot(east, north);
    const double cosine_angle = std::sin(p1) * std::sin(p2) + std::cos(p1) * std::cos(p2) * std::cos(dl);
    // Haversine avoids cancellation at small separation; the complementary
    // vector angle preserves precision near the antipode where a rounds to 1.
    if (a > 0.99) angle = std::atan2(sine_angle, cosine_angle);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    if (sine_angle <= degenerate_tolerance) {
        const bool same = cosine_angle >= 0.0;
        angle = same ? 0.0 : pi;
        return {radius_km * angle, nan, radius_km * (2.0 * pi - angle), nan,
                same ? BearingStatus::Coincident : BearingStatus::Antipodal};
    }
    const double bearing = azimuth(std::atan2(east, north));
    return {radius_km * angle, bearing, radius_km * (2.0 * pi - angle),
            std::fmod(bearing + 180.0, 360.0), BearingStatus::Defined};
}

std::string maidenhead(Location location, unsigned precision) {
    require_valid(location);
    if (precision != 2 && precision != 4 && precision != 6)
        throw std::invalid_argument("Maidenhead precision must be 2, 4, or 6 characters");
    const unsigned x=locator_axis(longitude(location.longitude),12.0,180.0);
    const unsigned y=locator_axis(location.latitude,24.0,90.0);
    std::string result;
    result += static_cast<char>('A'+x/240U);
    result += static_cast<char>('A'+y/240U);
    if(precision==2)return result;
    result += static_cast<char>('0'+x%240U/24U);
    result += static_cast<char>('0'+y%240U/24U);
    if(precision==4)return result;
    result += static_cast<char>('a'+x%24U);
    result += static_cast<char>('a'+y%24U);
    return result;
}
} // namespace hamclock
