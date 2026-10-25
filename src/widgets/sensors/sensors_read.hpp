#pragma once
// Local host sensors read from sysfs. Everything degrades to "absent" (no value)
// when the hardware or the file is missing, so the card never shows invented data.
#include <QByteArray>
#include <QString>
#include <optional>
#include <vector>
#include <utility>

namespace hamclock::sensors {

// "48312\n" (millidegrees Celsius, as in /sys/class/thermal/*/temp) -> 48.3.
// Rejects empty, non-numeric and implausible (outside -40..150 C) readings.
std::optional<double> parse_millidegrees(const QByteArray& text);

// Scans <thermal_root>/thermal_zone*/ and returns the CPU zone's temperature.
// Prefers a zone whose `type` looks like a CPU (cpu-thermal, x86_pkg_temp,
// soc_thermal), else the first zone that reads validly.
std::optional<double> read_cpu_temp(const QString& thermal_root = "/sys/class/thermal");

// /proc/net/wireless text -> signal level in dBm of the first interface with a
// plausible reading (-120..0). Absent on wired-only hosts and non-Linux systems.
std::optional<int> parse_wireless_dbm(const QByteArray& text);
std::optional<int> read_wifi_dbm(const QString& path = "/proc/net/wireless");

// Used fraction (0..1) of the filesystem holding `path`; absent when unreadable.
std::optional<double> read_disk_used_fraction(const QString& path = "/");
// Free space of that filesystem as short text ("12.4 GB free", "850 MB free"); formatting is pure.
QString format_free_bytes(qint64 bytes);
QString read_disk_free_text(const QString& path = "/");  // empty when unreadable

// BME280/BMP280 environment sensor via the kernel IIO driver
// (<root>/iio:device*/{name,in_temp_input,in_pressure_input,in_humidityrelative_input}).
// Units there: milli-degC, kPa, milli-%RH. Each field is absent when missing or implausible.
struct EnvReading {
    std::optional<double> temp_c, pressure_hpa, humidity_pct;
    bool any() const { return temp_c || pressure_hpa || humidity_pct; }
};
EnvReading read_env_sensor(const QString& iio_root = "/sys/bus/iio/devices");

// Adds user corrections (decimal text, e.g. "-1.5") to temperature and pressure.
// Empty, unparsable or implausible (|temp| > 20 C, |pressure| > 50 hPa) values are ignored.
void apply_offsets(EnvReading& reading, const QString& temp_offset_c, const QString& pressure_offset_hpa);

// Magnus-formula dew point (C); absent for humidity outside 1..100 %.
std::optional<double> dew_point_c(double temp_c, double humidity_pct);

// In-memory pressure samples (at most one per minute, last 3 h) for a trend word.
// Needs at least 30 minutes of history; the change is scaled to a 3 h equivalent
// and "Steady" lies within +-1 hPa of that, like the weather card's trend.
class PressureHistory {
public:
    void add(qint64 epoch_s, double hpa);
    std::optional<double> change_per_3h() const;
    QString trend_word() const;  // "Rising" / "Falling" / "Steady", empty if too little history
private:
    std::vector<std::pair<qint64, double>> samples_;
};

// "3 h: 18.2 to 23.5 °C" from a window low and high; empty when either is missing or inverted.
QString format_temp_range(std::optional<double> low, std::optional<double> high);

// Highest value seen in the last 3 h (in-memory, one sample per minute); used for the
// CPU temperature peak. Absent until a sample exists.
class PeakHistory {
public:
    void add(qint64 epoch_s, double value);
    std::optional<double> peak() const;
    std::optional<double> trough() const;  // lowest value in the window
private:
    std::vector<std::pair<qint64, double>> samples_;
};

} // namespace hamclock::sensors
