#include "xml_mini.hpp"
#include <cstdint>

namespace hcp::xml {
namespace {
constexpr size_t kMaxDepth = 64;

bool is_space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
bool is_name_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' ||
           c == '.' || c == ':' || static_cast<unsigned char>(c) >= 0x80;
}

void append_utf8(std::string& out, uint32_t cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

class Reader {
public:
    explicit Reader(std::string_view s) : s_(s) {}

    std::optional<Element> document() {
        if (!misc()) return std::nullopt;
        if (!starts("<")) return std::nullopt;
        Element root;
        if (!element(root, 0)) return std::nullopt;
        if (!misc() || pos_ != s_.size()) return std::nullopt; // only comments/PIs/space after the root
        return root;
    }

private:
    std::string_view s_;
    size_t pos_ = 0;

    bool starts(std::string_view t) const { return s_.substr(pos_, t.size()) == t; }
    void skip_space() {
        while (pos_ < s_.size() && is_space(s_[pos_])) ++pos_;
    }
    bool skip_past(std::string_view end) {
        const size_t at = s_.find(end, pos_);
        if (at == std::string_view::npos) return false;
        pos_ = at + end.size();
        return true;
    }

    // Space, comments, processing instructions and a DOCTYPE outside the root element.
    bool misc() {
        for (;;) {
            skip_space();
            if (starts("<?")) {
                if (!skip_past("?>")) return false;
            } else if (starts("<!--")) {
                if (!skip_past("-->")) return false;
            } else if (starts("<!DOCTYPE")) {
                if (s_.find('[', pos_) < s_.find('>', pos_)) return false; // internal subset not supported
                if (!skip_past(">")) return false;
            } else {
                return true;
            }
        }
    }

    bool name(std::string& out) {
        const size_t start = pos_;
        while (pos_ < s_.size() && is_name_char(s_[pos_])) ++pos_;
        if (pos_ == start) return false;
        out.assign(s_.substr(start, pos_ - start));
        return true;
    }

    // Text up to `stop` with references decoded; false on a bad reference or a stray '<'.
    bool text_until(char stop, std::string& out) {
        while (pos_ < s_.size() && s_[pos_] != stop) {
            const char c = s_[pos_];
            if (c == '<' && stop != '<') return false;
            if (c != '&') {
                out += c;
                ++pos_;
                continue;
            }
            const size_t semi = s_.find(';', pos_);
            if (semi == std::string_view::npos || semi - pos_ > 12) return false;
            const auto ref = s_.substr(pos_ + 1, semi - pos_ - 1);
            pos_ = semi + 1;
            if (ref == "amp") out += '&';
            else if (ref == "lt") out += '<';
            else if (ref == "gt") out += '>';
            else if (ref == "quot") out += '"';
            else if (ref == "apos") out += '\'';
            else if (ref.size() > 1 && ref[0] == '#') {
                const bool hex = ref[1] == 'x';
                uint32_t cp = 0;
                const auto digits = ref.substr(hex ? 2 : 1);
                if (digits.empty()) return false;
                for (char d : digits) {
                    int v = d >= '0' && d <= '9' ? d - '0' : hex && d >= 'a' && d <= 'f' ? d - 'a' + 10
                            : hex && d >= 'A' && d <= 'F' ? d - 'A' + 10 : -1;
                    if (v < 0 || cp > 0x10FFFF) return false;
                    cp = cp * (hex ? 16 : 10) + static_cast<uint32_t>(v);
                }
                if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
                append_utf8(out, cp);
            } else {
                return false; // undeclared entity
            }
        }
        return pos_ < s_.size();
    }

    bool element(Element& e, size_t depth) {
        if (depth > kMaxDepth) return false;
        ++pos_; // '<'
        if (!name(e.name)) return false;
        for (;;) { // attributes
            const size_t before = pos_;
            skip_space();
            if (starts("/>")) {
                pos_ += 2;
                return true;
            }
            if (starts(">")) {
                ++pos_;
                break;
            }
            if (pos_ == before) return false; // attributes must be separated by space
            std::string key, value;
            if (!name(key)) return false;
            skip_space();
            if (!starts("=")) return false;
            ++pos_;
            skip_space();
            if (pos_ >= s_.size() || (s_[pos_] != '"' && s_[pos_] != '\'')) return false;
            const char quote = s_[pos_++];
            if (!text_until(quote, value)) return false;
            ++pos_; // closing quote
            for (const auto& a : e.attributes)
                if (a.first == key) return false; // duplicate attribute
            e.attributes.emplace_back(std::move(key), std::move(value));
        }
        bool has_text = false;
        for (;;) { // content
            if (pos_ >= s_.size()) return false; // unclosed
            if (starts("</")) {
                pos_ += 2;
                std::string end;
                if (!name(end) || end != e.name) return false;
                skip_space();
                if (!starts(">")) return false;
                ++pos_;
                e.mixed = has_text && !e.children.empty();
                return true;
            }
            if (starts("<!--")) {
                if (!skip_past("-->")) return false;
            } else if (starts("<![CDATA[")) {
                pos_ += 9;
                const size_t end = s_.find("]]>", pos_);
                if (end == std::string_view::npos) return false;
                e.text.append(s_.substr(pos_, end - pos_));
                has_text = has_text || end > pos_;
                pos_ = end + 3;
            } else if (starts("<?")) {
                if (!skip_past("?>")) return false;
            } else if (starts("<")) {
                Element child;
                if (!element(child, depth + 1)) return false;
                e.children.push_back(std::move(child));
            } else {
                std::string chunk;
                if (!text_until('<', chunk)) return false;
                for (char c : chunk) has_text = has_text || !is_space(c);
                e.text += chunk;
            }
        }
    }
};
} // namespace

std::string Element::attribute(std::string_view key) const {
    for (const auto& a : attributes)
        if (a.first == key) return a.second;
    return {};
}

std::optional<Element> parse(std::string_view body) { return Reader(body).document(); }

} // namespace hcp::xml
