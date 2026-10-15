#include "radio_parse.hpp"
#include <cmath>

namespace hamclock::radio {

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

bool is_error(const QString& line) { return line.trimmed().startsWith("RPRT"); }

std::optional<double> parse_frequency_hz(const QString& line) {
    bool ok = false;
    const double hz = line.trimmed().toDouble(&ok);
    if (!ok || !std::isfinite(hz) || hz < 1000 || hz > 1.0e12) return std::nullopt;
    return hz;
}

std::optional<QString> parse_mode(const QString& line) {
    const QString m = line.trimmed();
    if (m.isEmpty() || m.size() > 12 || is_error(m)) return std::nullopt;
    for (const QChar c : m) if (!c.isLetterOrNumber() && c != '-') return std::nullopt;
    return m;
}

std::optional<int> parse_passband_hz(const QString& line) {
    bool ok = false;
    const int hz = line.trimmed().toInt(&ok);
    if (!ok || hz <= 0 || hz > 1000000) return std::nullopt;
    return hz;
}

QString format_passband(int hz) {
    if (hz <= 0) return QString();
    if (hz < 1000) return QString::number(hz) + " Hz";
    QString s = QString::number(hz / 1000.0, 'f', 2);
    while (s.endsWith('0')) s.chop(1);
    if (s.endsWith('.')) s.chop(1);
    return s + " kHz";
}

QString format_frequency(double hz) {
    if (!std::isfinite(hz) || hz <= 0) return QString();
    const double mhz = hz / 1.0e6;
    QString s = QString::number(mhz, 'f', 6);
    while (s.endsWith('0') && s.size() > s.indexOf('.') + 4) s.chop(1);
    return s + " MHz";
}

QString band_for(double hz) {
    struct B { double lo, hi; const char* name; };
    static const B bands[] = {
        {1.8e6, 2.0e6, "160m"}, {3.5e6, 4.0e6, "80m"}, {5.25e6, 5.45e6, "60m"}, {7.0e6, 7.3e6, "40m"},
        {10.1e6, 10.15e6, "30m"}, {14.0e6, 14.35e6, "20m"}, {18.068e6, 18.168e6, "17m"},
        {21.0e6, 21.45e6, "15m"}, {24.89e6, 24.99e6, "12m"}, {28.0e6, 29.7e6, "10m"},
        {50.0e6, 54.0e6, "6m"}, {144.0e6, 148.0e6, "2m"}, {420.0e6, 450.0e6, "70cm"}};
    for (const auto& b : bands) if (hz >= b.lo && hz <= b.hi) return b.name;
    return QString();
}

} // namespace hamclock::radio
