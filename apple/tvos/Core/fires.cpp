#include "fires.hpp"
#include "text_util.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace hcp::fires {
namespace {
std::vector<std::string_view> split(std::string_view s, char sep) {
    std::vector<std::string_view> out;
    size_t start = 0;
    for (;;) {
        const size_t at = s.find(sep, start);
        out.push_back(s.substr(start, at == std::string_view::npos ? std::string_view::npos : at - start));
        if (at == std::string_view::npos) return out;
        start = at + 1;
    }
}

int index_of(const std::vector<std::string_view>& header, std::string_view name) {
    const auto it = std::find(header.begin(), header.end(), name);
    return it == header.end() ? -1 : static_cast<int>(it - header.begin());
}
} // namespace

std::optional<std::vector<Detection>> parse(std::string_view body) {
    const auto lines = split(body, '\n');
    const std::string head = text::trimmed(lines[0]);
    const auto header = split(head, ',');
    const int lat = index_of(header, "latitude"), lon = index_of(header, "longitude"),
              date = index_of(header, "acq_date"), time = index_of(header, "acq_time"),
              conf = index_of(header, "confidence"), frp = index_of(header, "frp");
    if (std::min({lat, lon, date, time, conf, frp}) < 0) return std::nullopt;
    const size_t needed = static_cast<size_t>(std::max({lat, lon, date, time, conf, frp}));
    std::vector<Detection> out;
    for (size_t i = 1; i < lines.size(); ++i) {
        const std::string line = text::trimmed(lines[i]);
        const auto f = split(line, ',');
        if (f.size() <= needed) continue;
        const auto a = text::to_double(f[size_t(lat)]);
        const auto b = text::to_double(f[size_t(lon)]);
        const auto c = text::to_double(f[size_t(frp)]);
        std::string hhmm(f[size_t(time)]);
        if (hhmm.size() < 4) hhmm.insert(0, 4 - hhmm.size(), '0'); // rightJustified(4, '0')
        const auto when = text::parse_ymd(f[size_t(date)], hhmm.size() == 4 ? hhmm : std::string("bad"));
        if (!a || !b || !c || std::fabs(*a) > 90 || std::fabs(*b) > 180 || *c < 0 || !when) continue;
        Detection d;
        d.latitude = *a;
        d.longitude = *b;
        d.frp_mw = *c;
        d.acquired_ms = *when;
        const std::string level = text::lower(f[size_t(conf)]);
        d.confidence = level == "high" || level == "h" ? Detection::High
                       : level == "low" || level == "l" ? Detection::Low
                                                        : Detection::Nominal;
        out.push_back(d);
    }
    std::stable_sort(out.begin(), out.end(), [](const Detection& x, const Detection& y) { return x.frp_mw > y.frp_mw; });
    return out;
}

} // namespace hcp::fires
