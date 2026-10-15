#include "rotator_parse.hpp"
#include <cmath>

namespace hamclock::rotator {

Endpoint parse_endpoint(const QString& host, const QString& port, const Endpoint& fallback) {
    Endpoint e = fallback;
    const QString h = host.trimmed();
    bool hostOk = !h.isEmpty() && h.size() <= 253;
    for (const QChar c : h)
        if (!(c.isLetterOrNumber() && c.unicode() < 128) && c != '.' && c != '-' && c != ':') hostOk = false;
    if (hostOk) e.host = h;
    bool ok = false;
    const uint p = port.trimmed().toUInt(&ok);
    if (ok && p >= 1 && p <= 65535) e.port = static_cast<quint16>(p);
    return e;
}

namespace {
std::optional<double> number(const QString& line, double lo, double hi) {
    bool ok = false;
    const double v = line.trimmed().toDouble(&ok);
    if (!ok || !std::isfinite(v) || v < lo || v > hi) return std::nullopt;
    return v;
}
}

std::optional<double> parse_azimuth(const QString& line) { return number(line, 0, 360); }
std::optional<double> parse_elevation(const QString& line) { return number(line, -90, 90); }
bool is_error(const QString& line) { return line.trimmed().startsWith("RPRT"); }

QString format_degrees(double deg) {
    if (!std::isfinite(deg)) return {};
    return QString::number(static_cast<int>(std::lround(deg))) + QString::fromUtf8("°");
}

} // namespace hamclock::rotator
