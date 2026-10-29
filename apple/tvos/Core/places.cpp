#include "places.hpp"
#include "json_mini.hpp"
#include "text_util.hpp"
#include <cmath>

namespace hcp::places {

std::optional<std::vector<Place>> parse_nominatim(std::string_view body) {
    const auto doc = json::parse(body);
    if (!doc || !doc->is_array()) return std::nullopt;
    std::vector<Place> out;
    for (const auto& o : doc->array) {
        // Nominatim sends coordinates as strings; QJsonValue::toString() of a number is empty.
        const auto lat = text::to_double(o.at("lat").as_string());
        const auto lon = text::to_double(o.at("lon").as_string());
        if (!lat || !lon || std::fabs(*lat) > 90 || std::fabs(*lon) > 180) continue;
        out.push_back(Place{o.at("display_name").as_string(), *lat, *lon});
    }
    return out;
}

std::string open_meteo_timezone(std::string_view body) {
    const auto doc = json::parse(body);
    if (!doc || !doc->is_object()) return {};
    return text::trimmed(doc->at("timezone").as_string());
}

} // namespace hcp::places
