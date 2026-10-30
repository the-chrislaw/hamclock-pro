#pragma once
// Small Qt-free stand-ins for the QString/QDateTime conversions the Qt parsers rely on.
// ASCII only for case folding (every feed keyword compared this way is ASCII).
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace hcp::text {

std::string trimmed(std::string_view s);            // QString::trimmed (ASCII whitespace)
std::string lower(std::string_view s);
std::string upper(std::string_view s);
bool contains_ci(std::string_view haystack, std::string_view needle);

// QString::toDouble / toInt: optional surrounding whitespace, whole text must convert, finite.
std::optional<double> to_double(std::string_view s);
std::optional<int> to_int(std::string_view s);
// QJsonValue::toInt(def) on a double: the value when it is a whole number in int range, else def.
int whole_or(double v, int def);

// Civil UTC date/time -> epoch ms; nullopt for an impossible date or time.
std::optional<int64_t> civil_ms(int y, int mo, int d, int h, int mi, int s, int ms = 0);
// Fixed formats used by QDateTime::fromString with an explicit format, UTC.
std::optional<int64_t> parse_ymd(std::string_view date, std::string_view time); // "yyyy-MM-dd", "HH:mm[:ss]" or "HHmm"
// Qt::ISODate(WithMs): "yyyy-MM-ddTHH:mm[:ss[.fff...]]" then "Z", "+hh:mm", "+hhmm" or nothing (taken as UTC).
std::optional<int64_t> parse_iso8601_ms(std::string_view text);

} // namespace hcp::text
