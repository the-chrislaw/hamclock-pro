#include "alerts.hpp"
#include "json_mini.hpp"
#include "text_util.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <set>

namespace hcp::alerts {
namespace {
constexpr int64_t kHourMs = 3600 * 1000LL;

std::string_view trim_blanks(std::string_view s) { // [ \t]* on both sides, as the Qt regexes
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
    return s;
}

std::vector<std::string_view> lines(std::string_view text) {
    std::vector<std::string_view> out;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t end = text.find('\n', start);
        out.push_back(text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return out;
}

// "^Name:[ \t]*(.*?)[ \t]*$" on the first matching line.
std::string field(const std::vector<std::string_view>& message, std::string_view name) {
    for (auto line : message) {
        if (line.size() > name.size() && line.substr(0, name.size()) == name && line[name.size()] == ':')
            return std::string(trim_blanks(line.substr(name.size() + 1)));
    }
    return {};
}

std::string remove_all(std::string s, std::string_view what) {
    for (size_t at = s.find(what); at != std::string::npos; at = s.find(what, at)) s.erase(at, what.size());
    return s;
}

std::string crlf_to_lf(std::string s) { // QString::replace("\r\n", "\n")
    for (size_t at = s.find("\r\n"); at != std::string::npos; at = s.find("\r\n", at)) s.erase(at, 1);
    return s;
}

std::string lower(std::string_view s) {
    std::string out(s);
    for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

bool scale_at(const std::string& s, size_t at) {
    return at + 1 < s.size() && (s[at] == 'g' || s[at] == 's' || s[at] == 'r') && s[at + 1] >= '1' && s[at + 1] <= '5';
}

// "(?:Noaa Scale|NOAA Scale):\s*([GSR][1-5])|Category ([GSR][1-5])", case-insensitive, first match.
std::string find_scale(std::string_view message) {
    const std::string s = lower(message);
    for (size_t i = 0; i < s.size(); ++i) {
        if (s.compare(i, 11, "noaa scale:") == 0) {
            size_t at = i + 11;
            while (at < s.size() && std::isspace(static_cast<unsigned char>(s[at]))) ++at;
            if (scale_at(s, at)) return {static_cast<char>(std::toupper(s[at])), s[at + 1]};
        }
        if (s.compare(i, 9, "category ") == 0 && scale_at(s, i + 9))
            return {static_cast<char>(std::toupper(s[i + 9])), s[i + 10]};
    }
    return {};
}

struct Head {
    std::string kind, text;
};

// "^(ALERT|EXTENDED WARNING|WARNING|CANCEL WATCH|WATCH|SUMMARY):[ \t]*(.+?)[ \t]*$", first matching line.
bool headline(const std::vector<std::string_view>& message, Head& out) {
    static const std::array<std::string_view, 6> kinds = {"ALERT", "EXTENDED WARNING", "WARNING",
                                                          "CANCEL WATCH", "WATCH", "SUMMARY"};
    for (auto line : message) {
        for (auto k : kinds) {
            if (line.size() <= k.size() || line.substr(0, k.size()) != k || line[k.size()] != ':') continue;
            const auto text = trim_blanks(line.substr(k.size() + 1));
            if (text.empty()) continue;
            out = {std::string(k), std::string(text)};
            return true;
        }
    }
    return false;
}

std::string kind_word(const std::string& k) {
    if (k == "CANCEL WATCH") return "Cancel";
    if (k == "EXTENDED WARNING") return "Extended warning";
    return k.substr(0, 1) + lower(k.substr(1));
}
} // namespace

int64_t parse_swpc_time(std::string_view text) {
    static const std::array<std::string_view, 12> months = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    // Qt: trimmed().left(16) parsed as "yyyy MMM dd HHmm", else "yyyy MMM d HHmm".
    auto t = text;
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.front()))) t.remove_prefix(1);
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.back()))) t.remove_suffix(1);
    t = t.substr(0, 16);
    if (t.size() < 15 || t[4] != ' ' || t[8] != ' ') return 0;
    const auto year = hcp::text::to_int(t.substr(0, 4));
    const auto month = std::find(months.begin(), months.end(), t.substr(5, 3));
    const size_t day_len = t[10] == ' ' ? 1 : 2;
    if (t.size() != 9 + day_len + 1 + 4 || t[9 + day_len] != ' ') return 0;
    const auto day = t.substr(9, day_len), hhmm = t.substr(10 + day_len, 4);
    const auto all_digits = [](std::string_view s) {
        return std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; });
    };
    if (!year || month == months.end() || !all_digits(day) || !all_digits(hhmm)) return 0;
    const auto ms = hcp::text::civil_ms(*year, static_cast<int>(month - months.begin()) + 1, std::stoi(std::string(day)),
                                        std::stoi(std::string(hhmm.substr(0, 2))), std::stoi(std::string(hhmm.substr(2))), 0);
    return ms ? *ms : 0;
}

std::optional<std::vector<Alert>> parse(std::string_view body) {
    std::vector<Alert> out;
    const auto doc = json::parse(body);
    if (!doc || !doc->is_array()) return std::nullopt;
    for (const auto& row : doc->array) {
        if (!row.is_object()) continue;
        const std::string message = crlf_to_lf(std::string(row.at("message").as_string()));
        const auto text = lines(message);
        Head head;
        const int64_t issued = parse_swpc_time(remove_all(field(text, "Issue Time"), " UTC"));
        const std::string code(row.at("product_id").as_string());
        if (!headline(text, head) || issued == 0 || code.empty()) continue;
        Alert a;
        a.code = code;
        a.cancelled = head.kind == "CANCEL WATCH";
        a.kind = kind_word(head.kind);
        a.headline = head.text;
        a.issued_ms = issued;
        a.scale = find_scale(message);
        auto until = field(text, "Now Valid Until");
        if (until.empty()) until = field(text, "Valid To");
        a.until_ms = parse_swpc_time(remove_all(until, " UTC"));
        out.push_back(std::move(a));
    }
    std::stable_sort(out.begin(), out.end(), [](const Alert& x, const Alert& y) { return x.issued_ms > y.issued_ms; });
    return out;
}

std::vector<Alert> active(const std::vector<Alert>& all, int64_t now_ms) {
    std::vector<Alert> out;
    std::set<std::string> seen;
    for (const auto& a : all) { // newest first: the first message per product wins
        if (a.issued_ms > now_ms || !seen.insert(a.code).second) continue;
        if (a.cancelled) continue;
        if (a.until_ms != 0) {
            if (a.until_ms <= now_ms) continue;
        } else if (now_ms - a.issued_ms > (a.kind == "Watch" ? 48 : 24) * kHourMs) {
            continue;
        }
        out.push_back(a);
    }
    return out;
}

int level(const Alert& alert) {
    const int n = alert.scale.size() == 2 ? alert.scale[1] - '0' : 0;
    if (n >= 3) return 2;
    if (n >= 1 || alert.kind == "Warning" || alert.kind == "Extended warning" || alert.kind == "Watch") return 1;
    return 0;
}

} // namespace hcp::alerts
