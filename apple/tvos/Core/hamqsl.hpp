#pragma once
// Qt-free port of src/widgets/solarhamqsl/solar_hamqsl_parse (N0NBH HamQSL solar XML,
// https://www.hamqsl.com/solarxml.php). Same rules as the Qt parser, the behavioural oracle.
// The HF/VHF condition words are N0NBH's own calculation.
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::hamqsl {

struct HfBand {
    std::string name;  // "80m-40m"
    std::string day;   // "Poor" / "Fair" / "Good"
    std::string night;
};

struct VhfItem {
    std::string name;     // "E-Skip", "vhf-aurora"
    std::string location; // "europe", "north_america", "northern_hemi"
    std::string status;   // "Band Closed" or an opening
};

struct Report {
    int64_t updated_ms = 0; // UTC from <updated>; 0 when missing or unparsable
    std::vector<HfBand> hf; // only bands with both a day and a night value
    std::vector<VhfItem> vhf;
};

// Empty `hf` (and updated_ms 0) when the body is not a well-formed document.
Report parse(std::string_view body);

// 0 unknown, 1 poor, 2 fair, 3 good.
int condition_rank(std::string_view word);

// " 14 Oct 2026 0810 GMT" -> epoch ms; 0 when it does not parse.
int64_t parse_updated(std::string_view text);

} // namespace hcp::hamqsl
