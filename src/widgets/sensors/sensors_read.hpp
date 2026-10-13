#pragma once
// Local host sensors read from sysfs. Everything degrades to "absent" (no value)
// when the hardware or the file is missing, so the card never shows invented data.
#include <QString>
#include <optional>

namespace hamclock::sensors {

// "48312\n" (millidegrees Celsius, as in /sys/class/thermal/*/temp) -> 48.3.
// Rejects empty, non-numeric and implausible (outside -40..150 C) readings.
std::optional<double> parse_millidegrees(const QByteArray& text);

// Scans <thermal_root>/thermal_zone*/ and returns the CPU zone's temperature.
// Prefers a zone whose `type` looks like a CPU (cpu-thermal, x86_pkg_temp,
// soc_thermal), else the first zone that reads validly.
std::optional<double> read_cpu_temp(const QString& thermal_root = "/sys/class/thermal");

} // namespace hamclock::sensors
