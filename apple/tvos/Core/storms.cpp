#include "storms.hpp"
#include "json_mini.hpp"
#include "swpc.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace hcp::storms {
namespace {
// NHC sends numbers as strings ("35") but some fields as numbers; accept both.
bool number(const json::Value& v, double* out) {
    if (v.is_number()) { *out = v.number; return std::isfinite(*out); }
    if (v.type == json::Value::Type::String && !v.string.empty()) {
        char* end = nullptr;
        *out = std::strtod(v.string.c_str(), &end);
        return end && *end == '\0' && std::isfinite(*out);
    }
    return false;
}
std::string trimmed(std::string s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}
} // namespace

std::optional<std::vector<Storm>> parse(std::string_view body) {
    const auto doc = json::parse(body);
    if (!doc || !doc->is_object() || !doc->at("activeStorms").is_array()) return std::nullopt;
    std::vector<Storm> out;
    for (const auto& o : doc->at("activeStorms").array) {
        Storm s;
        double wind = 0, lat = 0, lon = 0, v = 0;
        s.name = trimmed(o.at("name").as_string());
        if (s.name.empty() || !number(o.at("intensity"), &wind) || wind < 0 ||
            !number(o.at("latitudeNumeric"), &lat) || !number(o.at("longitudeNumeric"), &lon) ||
            std::fabs(lat) > 90 || std::fabs(lon) > 180)
            continue;
        s.id = o.at("id").as_string();
        s.classification = o.at("classification").as_string();
        s.wind_kt = static_cast<int>(std::lround(wind));
        if (number(o.at("pressure"), &v) && v > 0) s.pressure_mb = static_cast<int>(std::lround(v));
        s.latitude = lat;
        s.longitude = lon;
        if (number(o.at("movementDir"), &v) && v >= 0 && v <= 360) s.heading_deg = static_cast<int>(std::lround(v)) % 360;
        if (number(o.at("movementSpeed"), &v) && v >= 0) s.speed_kt = static_cast<int>(std::lround(v));
        auto t = swpc::parse_iso_utc_ms(o.at("publicAdvisory").at("issuance").as_string());
        if (!t) t = swpc::parse_iso_utc_ms(o.at("lastUpdate").as_string());
        s.issued_ms = t.value_or(0);
        out.push_back(std::move(s));
    }
    std::stable_sort(out.begin(), out.end(), [](const Storm& a, const Storm& b) { return a.wind_kt > b.wind_kt; });
    return out;
}

std::string classification_text(const std::string& c) {
    if (c == "HU") return "Hurricane";
    if (c == "TS") return "Tropical storm";
    if (c == "TD") return "Tropical depression";
    if (c == "STS") return "Subtropical storm";
    if (c == "STD") return "Subtropical depression";
    if (c == "PTC") return "Potential cyclone";
    if (c == "PC") return "Post-tropical cyclone";
    if (c == "RE") return "Remnants";
    return "Tropical cyclone";
}

int category(int wind_kt) {
    if (wind_kt < 64) return 0;
    if (wind_kt < 83) return 1;
    if (wind_kt < 96) return 2;
    if (wind_kt < 113) return 3;
    if (wind_kt < 137) return 4;
    return 5;
}

std::string compass(int degrees) {
    if (degrees < 0) return {};
    static const char* const points[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    return points[((degrees % 360) * 8 + 180) / 360 % 8];
}

} // namespace hcp::storms
