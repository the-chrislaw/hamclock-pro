#include "hamqsl.hpp"
#include "text_util.hpp"
#include "xml_mini.hpp"
#include <algorithm>
#include <array>
#include <sstream>

namespace hcp::hamqsl {
namespace {
std::string trimmed(std::string_view s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return std::string(s.substr(first, last - first + 1));
}

struct Walk {
    Report out;
    bool error = false;

    // QXmlStreamReader::readElementText: text only; a child element is an error.
    std::string element_text(const xml::Element& e) {
        if (!e.children.empty()) error = true;
        return e.text;
    }

    void visit(const xml::Element& e, bool in_hf, bool in_vhf) {
        if (error) return;
        const auto& n = e.name;
        if (n == "updated") {
            out.updated_ms = parse_updated(element_text(e));
            return;
        }
        if (in_hf && n == "band") {
            const auto band = e.attribute("name"), time = e.attribute("time");
            const auto word = trimmed(element_text(e));
            if (band.empty() || word.empty() || (time != "day" && time != "night")) return;
            auto row = std::find_if(out.hf.begin(), out.hf.end(), [&](const HfBand& b) { return b.name == band; });
            if (row == out.hf.end()) row = out.hf.insert(out.hf.end(), HfBand{band, {}, {}});
            (time == "day" ? row->day : row->night) = word;
            return;
        }
        if (in_vhf && n == "phenomenon") {
            VhfItem item{e.attribute("name"), e.attribute("location"), trimmed(element_text(e))};
            if (!item.name.empty() && !item.status.empty()) out.vhf.push_back(std::move(item));
            return;
        }
        const bool hf = in_hf || n == "calculatedconditions";
        const bool vhf = in_vhf || n == "calculatedvhfconditions";
        for (const auto& c : e.children) visit(c, hf, vhf);
    }
};
} // namespace

int64_t parse_updated(std::string_view text) {
    static const std::array<std::string_view, 12> months = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    auto t = trimmed(text);
    if (t.size() < 3 || t.compare(t.size() - 3, 3, "GMT") != 0) return 0;
    t.resize(t.size() - 3);
    std::istringstream in(t);
    std::vector<std::string> parts;
    for (std::string p; in >> p;) parts.push_back(p);
    if (parts.size() != 4 || parts[3].size() != 4) return 0;
    const auto day = hcp::text::to_int(parts[0]), year = hcp::text::to_int(parts[2]);
    const auto month = std::find(months.begin(), months.end(), parts[1]);
    const auto hh = hcp::text::to_int(parts[3].substr(0, 2)), mm = hcp::text::to_int(parts[3].substr(2));
    if (!day || !year || month == months.end() || !hh || !mm || parts[0].size() > 2 || parts[2].size() != 4) return 0;
    const auto ms = hcp::text::civil_ms(*year, static_cast<int>(month - months.begin()) + 1, *day, *hh, *mm, 0);
    return ms ? *ms : 0;
}

Report parse(std::string_view body) {
    const auto doc = xml::parse(body);
    if (!doc) return {};
    Walk walk;
    walk.visit(*doc, false, false);
    if (walk.error) return {};
    auto& hf = walk.out.hf;
    // A band without both halves is incomplete: drop it rather than show a blank.
    hf.erase(std::remove_if(hf.begin(), hf.end(), [](const HfBand& b) { return b.day.empty() || b.night.empty(); }),
             hf.end());
    return walk.out;
}

int condition_rank(std::string_view word) {
    if (word == "Good") return 3;
    if (word == "Fair") return 2;
    if (word == "Poor") return 1;
    return 0;
}

} // namespace hcp::hamqsl
