#include "aprs_parse.hpp"
#include <cmath>

namespace hamclock::aprs {
namespace {

bool valid_call(const QString& call) {
    if (call.size() < 3 || call.size() > 9) return false;
    for (QChar c : call)
        if (!(c.isDigit() || (c.unicode() < 128 && c.isUpper()) || c == '-')) return false;
    return true;
}

// Ambiguity: trailing spaces stand for unknown digits and count as zero.
std::optional<double> degrees_minutes(QString text, int degree_digits, const QString& hemispheres) {
    if (text.size() != degree_digits + 6) return std::nullopt;
    const QChar hemi = text.at(text.size() - 1);
    if (!hemispheres.contains(hemi)) return std::nullopt;
    text.chop(1);
    text.replace(' ', '0');
    if (text.at(degree_digits + 2) != '.') return std::nullopt;
    bool ok_d = false, ok_m = false;
    const int deg = text.left(degree_digits).toInt(&ok_d);
    const double min = text.mid(degree_digits).toDouble(&ok_m);
    if (!ok_d || !ok_m || min < 0 || min >= 60) return std::nullopt;
    const double value = deg + min / 60.0;
    return hemi == hemispheres.at(0) ? value : -value;
}

std::optional<double> base91(const QString& four) {
    double v = 0;
    for (QChar c : four) {
        const int d = c.unicode() - 33;
        if (c.unicode() < 33 || c.unicode() > 123) return std::nullopt;
        v = v * 91 + d;
    }
    return v;
}

} // namespace

std::optional<Station> parse_packet(const QString& line) {
    if (line.isEmpty() || line.startsWith('#') || line.size() > 600) return std::nullopt;
    const int gt = line.indexOf('>'), colon = line.indexOf(':');
    if (gt < 1 || colon < gt) return std::nullopt;
    Station s;
    s.call = line.left(gt);
    if (!valid_call(s.call)) return std::nullopt;
    QString body = line.mid(colon + 1);
    if (body.isEmpty()) return std::nullopt;
    const QChar type = body.at(0);
    if (type != '!' && type != '=' && type != '/' && type != '@') return std::nullopt;
    body = body.mid(type == '/' || type == '@' ? 8 : 1); // timestamps are 7 characters
    if (body.size() < 10) return std::nullopt;
    if (body.at(0).isDigit() || body.at(0) == ' ') {
        if (body.size() < 19) return std::nullopt;
        const auto lat = degrees_minutes(body.left(8), 2, "NS");
        const auto lon = degrees_minutes(body.mid(9, 9), 3, "EW");
        if (!lat || !lon || *lat > 90 || *lon > 180) return std::nullopt;
        s.latitude = *lat; s.longitude = *lon;
        s.symbol = QString(body.at(8)) + body.at(18);
        s.comment = body.mid(19).trimmed();
    } else {
        const auto y = base91(body.mid(1, 4)), x = base91(body.mid(5, 4));
        if (!y || !x) return std::nullopt;
        s.latitude = 90.0 - *y / 380926.0;
        s.longitude = -180.0 + *x / 190463.0;
        s.symbol = QString(body.at(0)) + body.at(9);
        s.comment = body.mid(13).trimmed();
        if (std::fabs(s.latitude) > 90 || std::fabs(s.longitude) > 180) return std::nullopt;
    }
    s.comment.truncate(60);
    return s;
}

} // namespace hamclock::aprs
