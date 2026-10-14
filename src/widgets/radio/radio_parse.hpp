#pragma once
#include <QString>
#include <optional>

namespace hamclock::radio {

// Replies of rigctld's default (short) protocol to the read-only "f" and "m"
// commands. Error replies ("RPRT -n"), garbage and implausible values yield
// nullopt, never a made-up frequency.
std::optional<double> parse_frequency_hz(const QString& line);
std::optional<QString> parse_mode(const QString& line);
bool is_error(const QString& line);

// "14.074 MHz" style text, or e.g. "7.0305 MHz"; trailing zeros trimmed to kHz
// resolution at least. Empty for an invalid frequency.
QString format_frequency(double hz);
// HF/VHF band name for a frequency ("20m"), or empty outside the amateur bands.
QString band_for(double hz);

} // namespace hamclock::radio
