#pragma once
#include <QString>
#include <QtGlobal>
#include <optional>

namespace hamclock::rotator {

struct Endpoint {
    QString host;
    quint16 port = 0;
};
// Host and port from options.ini ([rotator] host / port). A host must be a plain
// name or address (letters, digits, '.', '-', ':'); a port must be 1..65535.
// Anything empty or invalid falls back to the default, field by field.
Endpoint parse_endpoint(const QString& host, const QString& port, const Endpoint& fallback);

// Replies of rotctld's default (short) protocol to the read-only "p" command:
// azimuth line then elevation line. Error replies ("RPRT -n"), garbage and
// out-of-range values yield nullopt, never a made-up bearing.
std::optional<double> parse_azimuth(const QString& line);    // 0..360
std::optional<double> parse_elevation(const QString& line);  // -90..90
bool is_error(const QString& line);

// "123°" style text (whole degrees), empty for an invalid value.
QString format_degrees(double deg);

} // namespace hamclock::rotator
