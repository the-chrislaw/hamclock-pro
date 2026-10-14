#include "json_mini.hpp"
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace hcp::json {
namespace {
constexpr int kMaxDepth = 64;

struct Parser {
    std::string_view s;
    size_t i = 0;

    void ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i; }
    bool eat(char c) { if (i < s.size() && s[i] == c) { ++i; return true; } return false; }
    bool literal(std::string_view word) {
        if (s.substr(i, word.size()) != word) return false;
        i += word.size();
        return true;
    }

    static void append_utf8(std::string& out, unsigned cp) {
        if (cp < 0x80) out += char(cp);
        else if (cp < 0x800) { out += char(0xC0 | cp >> 6); out += char(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) { out += char(0xE0 | cp >> 12); out += char(0x80 | (cp >> 6 & 0x3F)); out += char(0x80 | (cp & 0x3F)); }
        else { out += char(0xF0 | cp >> 18); out += char(0x80 | (cp >> 12 & 0x3F)); out += char(0x80 | (cp >> 6 & 0x3F)); out += char(0x80 | (cp & 0x3F)); }
    }
    bool hex4(unsigned& v) {
        if (i + 4 > s.size()) return false;
        v = 0;
        for (int k = 0; k < 4; ++k) {
            const char c = s[i++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= unsigned(c - '0');
            else if (c >= 'a' && c <= 'f') v |= unsigned(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= unsigned(c - 'A' + 10);
            else return false;
        }
        return true;
    }
    bool str(std::string& out) {
        if (!eat('"')) return false;
        while (i < s.size()) {
            const unsigned char c = static_cast<unsigned char>(s[i++]);
            if (c == '"') return true;
            if (c < 0x20) return false;
            if (c != '\\') { out += char(c); continue; }
            if (i >= s.size()) return false;
            switch (s[i++]) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'u': {
                unsigned cp = 0;
                if (!hex4(cp)) return false;
                if (cp >= 0xD800 && cp < 0xDC00) { // surrogate pair
                    unsigned lo = 0;
                    if (!literal("\\u") || !hex4(lo) || lo < 0xDC00 || lo > 0xDFFF) return false;
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) return false;
                append_utf8(out, cp);
                break;
            }
            default: return false;
            }
        }
        return false;
    }
    bool number(double& out) {
        const size_t start = i;
        eat('-');
        if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        if (s[i] == '0') ++i; else while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
        if (eat('.')) {
            if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) return false;
            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
        }
        if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
            ++i;
            if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
            if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) return false;
            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
        }
        out = std::strtod(std::string(s.substr(start, i - start)).c_str(), nullptr);
        return std::isfinite(out);
    }
    bool value(Value& v, int depth) {
        if (depth > kMaxDepth) return false;
        ws();
        if (i >= s.size()) return false;
        const char c = s[i];
        if (c == '{') {
            ++i; v.type = Value::Type::Object; ws();
            if (eat('}')) return true;
            for (;;) {
                ws();
                std::string key;
                if (!str(key)) return false;
                ws();
                if (!eat(':')) return false;
                Value child;
                if (!value(child, depth + 1)) return false;
                v.object[std::move(key)] = std::move(child);
                ws();
                if (eat(',')) continue;
                return eat('}');
            }
        }
        if (c == '[') {
            ++i; v.type = Value::Type::Array; ws();
            if (eat(']')) return true;
            for (;;) {
                Value child;
                if (!value(child, depth + 1)) return false;
                v.array.push_back(std::move(child));
                ws();
                if (eat(',')) continue;
                return eat(']');
            }
        }
        if (c == '"') { v.type = Value::Type::String; return str(v.string); }
        if (c == 't') { v.type = Value::Type::Bool; v.boolean = true; return literal("true"); }
        if (c == 'f') { v.type = Value::Type::Bool; return literal("false"); }
        if (c == 'n') return literal("null");
        v.type = Value::Type::Number;
        return number(v.number);
    }
};
} // namespace

const Value& Value::at(const std::string& key) const {
    static const Value null_value;
    if (type != Type::Object) return null_value;
    const auto it = object.find(key);
    return it == object.end() ? null_value : it->second;
}

std::optional<Value> parse(std::string_view text) {
    Parser p{text};
    Value v;
    if (!p.value(v, 0)) return std::nullopt;
    p.ws();
    if (p.i != text.size()) return std::nullopt;
    return v;
}

} // namespace hcp::json
