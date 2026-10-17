#pragma once
// Qt-free port of src/widgets/storms/storms_parse (NHC active-storm index, CurrentStorms.json).
// Same rules as the Qt parser, which stays the behavioural oracle in tests. Forecast tracks are not ported.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::storms {

struct Storm {
    std::string id;             // "ep212026"
    std::string name;
    std::string classification; // NHC code: TD, TS, HU, ...
    int wind_kt = 0;
    int pressure_mb = 0;        // 0 when not given
    double latitude = 0, longitude = 0;
    int heading_deg = -1;       // -1 unknown
    int speed_kt = -1;          // -1 unknown
    int64_t issued_ms = 0;      // advisory issuance, UTC; 0 when the index gives none
};

// Strongest first. nullopt when the body is not an NHC index; an empty list is valid.
// Rows without a name, wind or position are skipped.
std::optional<std::vector<Storm>> parse(std::string_view body);

std::string classification_text(const std::string& code); // "Hurricane", "Tropical storm", ...
int category(int wind_kt);                                  // Saffir-Simpson 1-5 for >= 64 kt, else 0
std::string compass(int degrees);                           // "NW"; empty for unknown

} // namespace hcp::storms
