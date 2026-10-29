#include "drap.hpp"
#include "text_util.hpp"
#include <algorithm>
#include <optional>

namespace hcp::drap {
namespace {
// "# Key : value" header line.
void header(const std::string& line, Grid& g, bool& has_time) {
    const auto colon = line.find(':');
    if (colon == std::string::npos) return;
    const std::string key = line.substr(0, colon);
    const std::string val = text::trimmed(std::string_view(line).substr(colon + 1));
    if (key.find("Valid At") != std::string::npos) {
        // QDateTime::fromString(val.left(16), "yyyy-MM-dd HH:mm")
        const std::string stamp = val.substr(0, 16);
        if (stamp.size() == 16 && stamp[10] == ' ') {
            if (const auto t = text::parse_ymd(std::string_view(stamp).substr(0, 10), std::string_view(stamp).substr(11))) {
                g.valid_ms = *t;
                has_time = true;
            }
        }
    } else if (key.find("X-RAY Message") != std::string::npos) {
        g.xray = val;
    } else if (key.find("Proton Message") != std::string::npos) {
        g.proton = val;
    }
}

// One "lat | v v v ..." row; false when it is not kCols non-negative numbers.
bool row(std::string_view values, std::vector<float>& out) {
    int n = 0;
    size_t i = 0;
    while (i < values.size()) {
        if (values[i] == ' ') { ++i; continue; }
        const size_t end = std::min(values.find(' ', i), values.size());
        const auto v = text::to_double(values.substr(i, end - i));
        if (!v || *v < 0) return false;
        out.push_back(static_cast<float>(*v));
        ++n;
        i = end;
    }
    return n == kCols;
}
} // namespace

Grid parse(std::string_view body) {
    Grid g;
    std::vector<float> values;
    int rows = 0;
    bool has_time = false;
    size_t start = 0;
    while (start <= body.size()) {
        const size_t nl = std::min(body.find('\n', start), body.size());
        const std::string line = text::trimmed(body.substr(start, nl - start));
        start = nl + 1;
        if (!line.empty() && line[0] == '#') { header(line, g, has_time); continue; }
        const auto bar = line.find('|');
        if (bar == std::string::npos) continue;
        if (!row(std::string_view(line).substr(bar + 1), values)) return Grid{};
        ++rows;
    }
    if (rows != kRows || !has_time) return Grid{};
    g.mhz = std::move(values);
    g.valid = true;
    return g;
}

double peak(const Grid& g) {
    if (!g.valid || g.mhz.empty()) return 0;
    return *std::max_element(g.mhz.begin(), g.mhz.end());
}

} // namespace hcp::drap
