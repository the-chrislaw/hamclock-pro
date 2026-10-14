#pragma once
// Minimal Qt-free JSON reader for the NOAA SWPC products (area I, tvOS core).
// Parses the whole RFC 8259 grammar into a small tree; rejects malformed input.
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hcp::json {

struct Value {
    enum class Type { Null, Bool, Number, String, Array, Object } type = Type::Null;
    bool boolean = false;
    double number = 0;
    std::string string;
    std::vector<Value> array;
    std::map<std::string, Value> object;

    bool is_number() const { return type == Type::Number; }
    bool is_object() const { return type == Type::Object; }
    bool is_array() const { return type == Type::Array; }
    // Missing key (or non-object) yields a shared null value.
    const Value& at(const std::string& key) const;
    std::string as_string() const { return type == Type::String ? string : std::string{}; }
};

std::optional<Value> parse(std::string_view text);

} // namespace hcp::json
