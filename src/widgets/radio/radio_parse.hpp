#pragma once
#include <QString>
#include <QtGlobal>
#include <optional>

namespace hamclock::radio {

struct Endpoint {
    QString host;
    quint16 port = 0;
};
// Host and port from options.ini ([radio] host / port). A host must be a plain
// name or address (letters, digits, '.', '-', ':'); a port must be 1..65535.
// Anything empty or invalid falls back to the default, field by field.
Endpoint parse_endpoint(const QString& host, const QString& port, const Endpoint& fallback);

// Replies of rigctld's default (short) protocol to the read-only "f" and "m"
// commands. Error replies ("RPRT -n"), garbage and implausible values yield
// nullopt, never a made-up frequency.
std::optional<double> parse_frequency_hz(const QString& line);
std::optional<QString> parse_mode(const QString& line);
bool is_error(const QString& line);
// Passband width in Hz from the second line of rigctld's "m" reply; nullopt for
// errors, zero (rig default) or implausible values (> 1 MHz).
std::optional<int> parse_passband_hz(const QString& line);
// "2.4 kHz" / "500 Hz" / "15 kHz"; empty for an invalid width.
QString format_passband(int hz);

// "14.074 MHz" style text, or e.g. "7.0305 MHz"; trailing zeros trimmed to kHz
// resolution at least. Empty for an invalid frequency.
QString format_frequency(double hz);
// HF/VHF band name for a frequency ("20m"), or empty outside the amateur bands.
QString band_for(double hz);

} // namespace hamclock::radio
