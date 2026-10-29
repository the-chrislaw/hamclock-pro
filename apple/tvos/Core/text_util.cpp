#include "text_util.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>

namespace hcp::text {
namespace {
bool space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; }
bool digit(char c) { return c >= '0' && c <= '9'; }

// Reads exactly n digits at s[i]; advances i.
bool digits(std::string_view s, size_t& i, size_t n, int& out) {
    if (i + n > s.size()) return false;
    out = 0;
    for (size_t k = 0; k < n; ++k, ++i) {
        if (!digit(s[i])) return false;
        out = out * 10 + (s[i] - '0');
    }
    return true;
}

int64_t days_from_civil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = unsigned(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + int64_t(doe) - 719468;
}

int days_in_month(int y, int m) {
    static constexpr int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    return m == 2 && leap ? 29 : days[m - 1];
}

bool date_part(std::string_view s, size_t& i, int& y, int& mo, int& d) {
    if (!digits(s, i, 4, y) || i >= s.size() || s[i++] != '-' || !digits(s, i, 2, mo) || i >= s.size() ||
        s[i++] != '-' || !digits(s, i, 2, d))
        return false;
    return true;
}
} // namespace

std::string trimmed(std::string_view s) {
    while (!s.empty() && space(s.front())) s.remove_prefix(1);
    while (!s.empty() && space(s.back())) s.remove_suffix(1);
    return std::string(s);
}

std::string lower(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return out;
}

std::string upper(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return char(std::toupper(c)); });
    return out;
}

bool contains_ci(std::string_view haystack, std::string_view needle) {
    return lower(haystack).find(lower(needle)) != std::string::npos;
}

std::optional<double> to_double(std::string_view s) {
    const std::string t = trimmed(s);
    if (t.empty()) return std::nullopt;
    char* end = nullptr;
    errno = 0;
    const double v = std::strtod(t.c_str(), &end);
    if (!end || *end != '\0' || errno == ERANGE || !std::isfinite(v)) return std::nullopt;
    return v;
}

std::optional<int> to_int(std::string_view s) {
    const std::string t = trimmed(s);
    if (t.empty()) return std::nullopt;
    char* end = nullptr;
    errno = 0;
    const long v = std::strtol(t.c_str(), &end, 10);
    if (!end || *end != '\0' || errno == ERANGE || v < INT_MIN || v > INT_MAX) return std::nullopt;
    return static_cast<int>(v);
}

int whole_or(double v, int def) {
    if (!std::isfinite(v) || v < INT_MIN || v > INT_MAX || std::trunc(v) != v) return def;
    return static_cast<int>(v);
}

std::optional<int64_t> civil_ms(int y, int mo, int d, int h, int mi, int s, int ms) {
    if (mo < 1 || mo > 12 || d < 1 || d > days_in_month(y, mo) || h < 0 || h > 23 || mi < 0 || mi > 59 || s < 0 ||
        s > 59 || ms < 0 || ms > 999)
        return std::nullopt;
    return ((days_from_civil(y, unsigned(mo), unsigned(d)) * 24 + h) * 60 + mi) * 60000 + int64_t(s) * 1000 + ms;
}

std::optional<int64_t> parse_ymd(std::string_view date, std::string_view time) {
    size_t i = 0;
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
    if (!date_part(date, i, y, mo, d) || i != date.size()) return std::nullopt;
    size_t j = 0;
    if (time.size() == 4) {
        if (!digits(time, j, 2, h) || !digits(time, j, 2, mi)) return std::nullopt;
    } else {
        if (!digits(time, j, 2, h) || j >= time.size() || time[j++] != ':' || !digits(time, j, 2, mi)) return std::nullopt;
        if (j < time.size() && (time[j++] != ':' || !digits(time, j, 2, s))) return std::nullopt;
        if (j != time.size()) return std::nullopt;
    }
    return civil_ms(y, mo, d, h, mi, s);
}

std::optional<int64_t> parse_iso8601_ms(std::string_view text) {
    const std::string t = trimmed(text);
    const std::string_view v(t);
    size_t i = 0;
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0, ms = 0;
    if (!date_part(v, i, y, mo, d) || i >= v.size() || v[i++] != 'T' || !digits(v, i, 2, h) || i >= v.size() ||
        v[i++] != ':' || !digits(v, i, 2, mi))
        return std::nullopt;
    if (i < v.size() && v[i] == ':') {
        ++i;
        if (!digits(v, i, 2, s)) return std::nullopt;
        if (i < v.size() && (v[i] == '.' || v[i] == ',')) {
            ++i;
            if (i >= v.size() || !digit(v[i])) return std::nullopt;
            int scale = 100;
            for (; i < v.size() && digit(v[i]); ++i, scale /= 10) ms += scale * (v[i] - '0');
        }
    }
    int64_t offset_min = 0;
    if (i < v.size() && v[i] == 'Z') {
        ++i;
    } else if (i < v.size() && (v[i] == '+' || v[i] == '-')) {
        const int sign = v[i++] == '-' ? -1 : 1;
        int oh = 0, om = 0;
        if (!digits(v, i, 2, oh)) return std::nullopt;
        if (i < v.size() && v[i] == ':') ++i;
        if (i < v.size() && !digits(v, i, 2, om)) return std::nullopt;
        if (oh > 23 || om > 59) return std::nullopt;
        offset_min = sign * (oh * 60 + om);
    }
    if (i != v.size()) return std::nullopt;
    const auto base = civil_ms(y, mo, d, h, mi, s, ms);
    if (!base) return std::nullopt;
    return *base - offset_min * 60000;
}

} // namespace hcp::text
