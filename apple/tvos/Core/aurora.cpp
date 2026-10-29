#include "aurora.hpp"
#include "json_mini.hpp"
#include "text_util.hpp"
#include <algorithm>

namespace hcp::aurora {

Grid parse(std::string_view body) {
    Grid g;
    const auto doc = json::parse(body);
    if (!doc || !doc->is_object()) return g;
    g.observed_ms = text::parse_iso8601_ms(doc->at("Observation Time").as_string()).value_or(0);
    g.forecast_ms = text::parse_iso8601_ms(doc->at("Forecast Time").as_string()).value_or(0);
    g.cells.assign(static_cast<size_t>(kLons * kLats), 0);
    int used = 0;
    for (const auto& c : doc->at("coordinates").array) {
        if (!c.is_array() || c.array.size() < 3 || !c.array[0].is_number() || !c.array[1].is_number() ||
            !c.array[2].is_number())
            continue;
        // QJsonValue::toInt(): a fractional number reads as 0, as in the Qt parser.
        const int lon = text::whole_or(c.array[0].number, 0);
        const int lat = text::whole_or(c.array[1].number, 0);
        const int p = text::whole_or(c.array[2].number, 0);
        if (lon < 0 || lon > 359 || lat < -90 || lat > 90) continue;
        g.cells[static_cast<size_t>((lat + 90) * kLons + lon)] = static_cast<unsigned char>(std::clamp(p, 0, 100));
        ++used;
    }
    g.valid = used > 0 && g.observed_ms != 0;
    return g;
}

int peak(const Grid& g, bool north) {
    if (!g.valid) return 0;
    int best = 0;
    for (int lat = north ? 0 : -90; lat <= (north ? 90 : -1); ++lat)
        for (int lon = 0; lon < kLons; ++lon) best = std::max(best, g.at(lat, lon));
    return best;
}

} // namespace hcp::aurora
