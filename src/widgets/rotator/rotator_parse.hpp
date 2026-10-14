#pragma once
#include <QString>
#include <optional>

namespace hamclock::rotator {

// Replies of rotctld's default (short) protocol to the read-only "p" command:
// azimuth line then elevation line. Error replies ("RPRT -n"), garbage and
// out-of-range values yield nullopt, never a made-up bearing.
std::optional<double> parse_azimuth(const QString& line);    // 0..360
std::optional<double> parse_elevation(const QString& line);  // -90..90
bool is_error(const QString& line);

// "123°" style text (whole degrees), empty for an invalid value.
QString format_degrees(double deg);

} // namespace hamclock::rotator
