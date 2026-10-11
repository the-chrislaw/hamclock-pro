#include "noaa_kp_parser.hpp"
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QSet>
#include <QStringDecoder>
#include <cmath>
#include <limits>
namespace hamclock {
    namespace {
        using Error = NoaaKpParseError;

        bool space(char c) {
            return c == ' ' || c == '\n' || c == '\r' || c == '\t';
        }
        int hex(char c) {
            if (c >= '0' && c <= '9') {
                return c-'0';
            }
            if (c >= 'a' && c <= 'f') {
                return c-'a'+10;
            }
            if (c >= 'A' && c <= 'F') {
                return c-'A'+10;
            }
            return -1;
        }
        bool codepoint(std::string_view s, std::size_t pos, unsigned& code) {
            if (s.size()-pos<4) {
                return false;
            }
            code = 0;
            for (unsigned n = 0;n<4;++n) {
                int h = hex(s[pos+n]);
                if (h<0) {
                    return false;
                }
                code = code*16+static_cast<unsigned>(h);
            }
            return true;
        }
        struct Frame {
            char kind;
            QSet<QString> keys;
            QString last_key;
        };
        // RFC 8259 number grammar; Qt/locale conversion is not a grammar gate.
        bool number_grammar(std::string_view token) {
            std::size_t pos = 0;
            auto digit = [](char c) { return c >= '0' && c <= '9'; };
            if (pos < token.size() && token[pos] == '-') { ++pos; }
            if (pos == token.size()) { return false; }
            if (token[pos] == '0') { ++pos; }
            else {
                if (token[pos] < '1' || token[pos] > '9') { return false; }
                while (pos < token.size() && digit(token[pos])) { ++pos; }
            }
            if (pos < token.size() && token[pos] == '.') {
                ++pos;
                const auto start = pos;
                while (pos < token.size() && digit(token[pos])) { ++pos; }
                if (pos == start) { return false; }
            }
            if (pos < token.size() && (token[pos] == 'e' || token[pos] == 'E')) {
                ++pos;
                if (pos < token.size() && (token[pos] == '+' || token[pos] == '-')) { ++pos; }
                const auto start = pos;
                while (pos < token.size() && digit(token[pos])) { ++pos; }
                if (pos == start) { return false; }
            }
            return pos == token.size();
        }
        Error preflight(std::string_view s) {
            if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xef && static_cast<unsigned char>(s[1]) == 0xbb && static_cast<unsigned char>(s[2]) == 0xbf) {
                return Error::InvalidJson;
            }
            QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::Stateless);
            const auto decoded = decoder(QByteArrayView(s.data(), static_cast<qsizetype>(s.size())));
            QString checked = decoded;
            if (decoder.hasError() || (!checked.isEmpty() && checked.front() == QChar(0xfeff))) {
                return Error::InvalidJson;
            }
            std::vector<Frame> stack;
            for (std::size_t i = 0;i<s.size();) {
                char c = s[i];
                if (c == '{' || c == '[') {
                    if (stack.size() >= kNoaaKpMaximumDepth) {
                        return Error::TooDeep;
                    }
                    stack.push_back( {
                        c, {}, {}
                    });
                    ++i;
                    continue;
                }
                if (c == '}' || c == ']') {
                    if (stack.empty() || stack.back().kind != (c == '}'?'{':'[')) {
                        return Error::InvalidJson;
                    }
                    stack.pop_back();
                    ++i;
                    continue;
                }
                if (c == '"') {
                    auto begin = i++;
                    bool closed = false;
                    while (i<s.size()) {
                        if (s[i] == '"') {
                            ++i;
                            closed = true;
                            break;
                        }
                        if (static_cast<unsigned char>(s[i])<32) {
                            return Error::InvalidJson;
                        }
                        if (s[i] == '\\') {
                            ++i;
                            if (i == s.size()) {
                                return Error::InvalidJson;
                            }
                            if (s[i] == 'u') {
                                unsigned code = 0;
                                if (!codepoint(s, i+1, code)) {
                                    return Error::InvalidJson;
                                }
                                i+= 5;
                                if (code >= 0xd800 && code <= 0xdbff) {
                                    if (s.size()-i<6 || s[i] != '\\' || s[i+1] != 'u') {
                                        return Error::InvalidJson;
                                    }
                                    unsigned low = 0;
                                    if (!codepoint(s, i+2, low) || low<0xdc00 || low>0xdfff) {
                                        return Error::InvalidJson;
                                    }
                                    i+= 6;
                                }
                                else if (code >= 0xdc00 && code <= 0xdfff)return Error::InvalidJson;
                                continue;
                            }
                            if (std::string_view("\"\\/bfnrt").find(s[i]) == std::string_view::npos) {
                                return Error::InvalidJson;
                            }
                        }
                        ++i;
                    }
                    if (!closed) {
                        return Error::InvalidJson;
                    }
                    std::size_t next = i;
                    while (next<s.size() && space(s[next]))++next;
                    if (next<s.size() && s[next] == ':') {
                        if (stack.empty() || stack.back().kind != '{') {
                            return Error::InvalidJson;
                        }
                        QByteArray token("[");
                        token.append(s.data()+begin, static_cast<qsizetype>(i-begin));
                        token.append(']');
                        QJsonParseError error;
                        auto keydoc = QJsonDocument::fromJson(token, &error);
                        if (error.error != QJsonParseError::NoError || !keydoc.isArray() || keydoc.array().size() != 1 || !keydoc.array()[0].isString()) {
                            return Error::InvalidJson;
                        }
                        QString key = keydoc.array()[0].toString();
                        if (static_cast<std::size_t>(key.toUtf8().size())>kNoaaKpMaximumKeyBytes) {
                            return Error::InvalidKey;
                        }
                        auto& f = stack.back();
                        if (f.keys.contains(key)) {
                            return Error::DuplicateKey;
                        }
                        if (static_cast<std::size_t>(f.keys.size()) >= kNoaaKpMaximumMembers) {
                            return Error::TooManyMembers;
                        }
                        f.keys.insert(key);
                        f.last_key = key;
                    }
                    continue;
                }
                if (c == '-' || c == '.' || c == '+' || (c >= '0' && c <= '9')) {
                    auto begin = i;
                    while (i<s.size() && !space(s[i]) && s[i] != ',' && s[i] != ']' && s[i] != '}')++i;
                    auto token = s.substr(begin, i-begin);
                    if (token.size()>kNoaaKpMaximumNumberBytes || !number_grammar(token)) {
                        return Error::InvalidNumber;
                    }
                    bool ok = false;
                    double v = QLocale::c().toDouble(QString::fromLatin1(token.data(), static_cast<qsizetype>(token.size())), &ok);
                    bool nonzero = false;
                    for (char digit:token) {
                        if (digit == 'e' || digit == 'E')break;
                        if (digit >= '1' && digit <= '9')nonzero = true;
                    }
                    if (!ok || !std::isfinite(v) || (v == 0 && nonzero)) {
                        return Error::InvalidNumber;
                    }
                    if (stack.size() == 2 && stack.back().kind == '{' && (stack.back().last_key == QStringLiteral("a_running") || stack.back().last_key == QStringLiteral("station_count")) && token.find_first_of(".eE") != std::string_view::npos) {
                        return Error::InvalidNumber;
                    }
                    continue;
                }
                ++i;
            }
            return stack.empty()?Error::None:Error::InvalidJson;
        }
        bool timestamp(const std::string& s) {
            if (s.size() != 19) {
                return false;
            }
            for (std::size_t i = 0;i<s.size();++i) {
                if (i == 4 || i == 7) {
                    if (s[i] != '-') {
                        return false;
                    }
                }
                else if (i == 10) {
                    if (s[i] != 'T') {
                        return false;
                    }
                }
                else if (i == 13 || i == 16) {
                    if (s[i] != ':') {
                        return false;
                    }
                }
                else if (s[i]<'0' || s[i]>'9')return false;
            }
            auto number = [&](std::size_t i, std::size_t n) {
                unsigned v = 0;
                for (std::size_t j = 0;j<n;++j)v = v*10+static_cast<unsigned>(s[i+j]-'0');
                return v;
            };
            unsigned year = number(0, 4), month = number(5, 2), day = number(8, 2);
            if (year == 0 || month<1 || month>12) {
                return false;
            }
            constexpr unsigned days[] =  {
                31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
            };
            unsigned limit = days[month-1];
            if (month == 2 && year%4 == 0 && (year%100 != 0 || year%400 == 0))++limit;
            return day >= 1 && day <= limit && number(11, 2)<24 && number(14, 2)<60 && number(17, 2)<60;
        }
        bool integer(const QJsonValue& v, std::optional<std::int32_t>& out) {
            if (v.isNull()) {
                out.reset();
                return true;
            }
if(!v.isDouble())return false;
            double x = v.toDouble();
            if (!std::isfinite(x) || x<0 || x>std::numeric_limits<std::int32_t>::max() || std::trunc(x) != x) {
                return false;
            }
            out = static_cast<std::int32_t>(x);
            return true;
        }
        NoaaKpParseResult fail(Error error) {
            return {
                error, nullptr
            };
        }
    }
    NoaaKpParseResult parse_noaa_kp(std::string_view bytes, std::string_view label) {
        if (label.empty() || label.size()>96) {
            return fail(Error::InvalidProvenance);
        }
        for (unsigned char c:label)if(c<32 || c>126)return fail(Error::InvalidProvenance);
        if (bytes.empty()) {
            return fail(Error::EmptyInput);
        }
        if (bytes.size()>kNoaaKpMaximumBytes) {
            return fail(Error::TooManyBytes);
        }
        auto pre = preflight(bytes);
        if (pre != Error::None) {
            return fail(pre);
        }
        QJsonParseError error;
        auto json = QJsonDocument::fromJson(QByteArray(bytes.data(), static_cast<qsizetype>(bytes.size())), &error);
        if (error.error != QJsonParseError::NoError) {
            return fail(Error::InvalidJson);
        }
        if (!json.isArray()) {
            return fail(Error::WrongRoot);
        }
        auto rows = json.array();
        if (static_cast<std::size_t>(rows.size())>kNoaaKpMaximumRows) {
            return fail(Error::TooManyRows);
        }
        NoaaKpRawDocument document;
        document.provenance_label = std::string(label);
        document.rows.reserve(static_cast<std::size_t>(rows.size()));
        for (const auto& value:rows) {
            if (!value.isObject()) {
                return fail(Error::InvalidRow);
            }
            auto object = value.toObject();
            for (auto key: {
                "time_tag","Kp","a_running","station_count"
            })if(!object.contains(QLatin1String(key)))return fail(Error::MissingField);
            if (!object.value(QStringLiteral("time_tag")).isString()) {
                return fail(Error::InvalidTimestamp);
            }
            NoaaKpRawRow row;
            row.raw_time_tag = object.value(QStringLiteral("time_tag")).toString().toStdString();
            if (!timestamp(row.raw_time_tag)) {
                return fail(Error::InvalidTimestamp);
            }
            auto kp = object.value(QStringLiteral("Kp"));
            if (!kp.isNull()) {
                if (!kp.isDouble()) {
                    return fail(Error::InvalidNumber);
                }
                double x = kp.toDouble();
                if (!std::isfinite(x) || x<0 || x>9) {
                    return fail(Error::InvalidNumber);
                }
                row.kp = x;
            }
            if (!integer(object.value(QStringLiteral("a_running")), row.a_running) || !integer(object.value(QStringLiteral("station_count")), row.station_count)) {
                return fail(Error::InvalidNumber);
            }
            row.ignored_field_count = static_cast<std::size_t>(object.size())-4;
            document.rows.push_back(std::move(row));
        }
        return {
            Error::None, std::make_shared<const NoaaKpRawDocument>(std::move(document))
        };
    }
} // namespace hamclock
