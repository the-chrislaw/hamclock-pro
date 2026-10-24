#include "sensors_read.hpp"
#include <QDir>
#include <QFile>
#include <QStorageInfo>
#include <cmath>
#include <algorithm>

namespace hamclock::sensors {

std::optional<double> parse_millidegrees(const QByteArray& text) {
    bool ok = false;
    const double milli = text.trimmed().toDouble(&ok);
    if (!ok) return std::nullopt;
    const double c = milli / 1000.0;
    if (c < -40 || c > 150) return std::nullopt;
    return c;
}

static QByteArray slurp(const QString& path) {
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.read(256) : QByteArray();
}

std::optional<double> read_cpu_temp(const QString& thermal_root) {
    const QDir root(thermal_root);
    QStringList zones = root.entryList({"thermal_zone*"}, QDir::Dirs | QDir::NoDotAndDotDot);
    std::sort(zones.begin(), zones.end());
    std::optional<double> fallback;
    for (const QString& z : zones) {
        const auto t = parse_millidegrees(slurp(root.filePath(z + "/temp")));
        if (!t) continue;
        const QByteArray type = slurp(root.filePath(z + "/type")).trimmed().toLower();
        if (type.contains("cpu") || type.contains("x86_pkg") || type.contains("soc")) return t;
        if (!fallback) fallback = t;
    }
    return fallback;
}

std::optional<int> parse_wireless_dbm(const QByteArray& text) {
    const QList<QByteArray> lines = text.split('\n');
    for (int i = 2; i < lines.size(); ++i) { // two header lines
        const QByteArray line = lines[i].trimmed();
        const int colon = line.indexOf(':');
        if (colon <= 0) continue;
        const QList<QByteArray> cols = line.mid(colon + 1).simplified().split(' ');
        if (cols.size() < 3) continue; // status, link, level, ...
        bool ok = false;
        const double level = cols[2].toDouble(&ok);
        if (!ok || level < -120 || level > 0) continue;
        return int(std::lround(level));
    }
    return std::nullopt;
}

std::optional<int> read_wifi_dbm(const QString& path) {
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? parse_wireless_dbm(f.read(4096)) : std::nullopt;
}

std::optional<double> read_disk_used_fraction(const QString& path) {
    const QStorageInfo info(path);
    if (!info.isValid() || !info.isReady() || info.bytesTotal() <= 0) return std::nullopt;
    return 1.0 - double(info.bytesAvailable()) / double(info.bytesTotal());
}

QString format_free_bytes(qint64 bytes) {
    if (bytes < 0) return {};
    const double gb = double(bytes) / 1e9;
    if (gb >= 100) return QString("%1 GB free").arg(std::llround(gb));
    if (gb >= 1) return QString("%1 GB free").arg(gb, 0, 'f', 1);
    return QString("%1 MB free").arg(std::llround(double(bytes) / 1e6));
}

QString read_disk_free_text(const QString& path) {
    const QStorageInfo info(path);
    if (!info.isValid() || !info.isReady() || info.bytesTotal() <= 0) return {};
    return format_free_bytes(info.bytesAvailable());
}

EnvReading read_env_sensor(const QString& iio_root) {
    const QDir root(iio_root);
    QStringList devs = root.entryList({"iio:device*"}, QDir::Dirs | QDir::NoDotAndDotDot);
    std::sort(devs.begin(), devs.end());
    for (const QString& d : devs) {
        const QByteArray name = slurp(root.filePath(d + "/name")).trimmed().toLower();
        if (!name.contains("bme280") && !name.contains("bmp280")) continue;
        auto num = [&](const char* file, double scale, double lo, double hi) -> std::optional<double> {
            bool ok = false;
            const double v = slurp(root.filePath(d + "/" + file)).trimmed().toDouble(&ok) * scale;
            return ok && v >= lo && v <= hi ? std::optional<double>(v) : std::nullopt;
        };
        EnvReading r;
        r.temp_c = num("in_temp_input", 0.001, -40, 85);
        r.pressure_hpa = num("in_pressure_input", 10.0, 300, 1100); // kPa -> hPa
        r.humidity_pct = num("in_humidityrelative_input", 0.001, 0, 100);
        if (r.any()) return r;
    }
    return {};
}

std::optional<double> dew_point_c(double t, double rh) {
    if (rh < 1 || rh > 100) return std::nullopt;
    const double a = 17.62, b = 243.12, g = std::log(rh / 100.0) + a * t / (b + t);
    return b * g / (a - g);
}

void PressureHistory::add(qint64 t, double hpa) {
    if (!samples_.empty() && t - samples_.back().first < 60) return;
    if (!samples_.empty() && t < samples_.back().first) samples_.clear();  // clock stepped back
    samples_.emplace_back(t, hpa);
    while (samples_.size() > 1 && t - samples_.front().first > 3 * 3600) samples_.erase(samples_.begin());
}

std::optional<double> PressureHistory::change_per_3h() const {
    if (samples_.size() < 2) return std::nullopt;
    const qint64 span = samples_.back().first - samples_.front().first;
    if (span < 30 * 60) return std::nullopt;
    return (samples_.back().second - samples_.front().second) * (3.0 * 3600.0 / static_cast<double>(span));
}

QString PressureHistory::trend_word() const {
    const auto c = change_per_3h();
    if (!c) return QString();
    return *c > 1.0 ? "Rising" : *c < -1.0 ? "Falling" : "Steady";
}

void PeakHistory::add(qint64 t, double v) {
    if (!samples_.empty() && t - samples_.back().first < 60) return;
    if (!samples_.empty() && t < samples_.back().first) samples_.clear();  // clock stepped back
    samples_.emplace_back(t, v);
    while (samples_.size() > 1 && t - samples_.front().first > 3 * 3600) samples_.erase(samples_.begin());
}

std::optional<double> PeakHistory::peak() const {
    if (samples_.empty()) return std::nullopt;
    double m = samples_.front().second;
    for (const auto& s : samples_) m = std::max(m, s.second);
    return m;
}

std::optional<double> PeakHistory::trough() const {
    if (samples_.empty()) return std::nullopt;
    double m = samples_.front().second;
    for (const auto& s : samples_) m = std::min(m, s.second);
    return m;
}

} // namespace hamclock::sensors

namespace hamclock::sensors {
namespace {
std::optional<double> parse_offset(const QString& text, double limit) {
    bool ok = false;
    const double v = text.trimmed().toDouble(&ok);
    return ok && std::isfinite(v) && std::abs(v) <= limit ? std::optional<double>(v) : std::nullopt;
}
}
void apply_offsets(EnvReading& reading, const QString& temp_offset_c, const QString& pressure_offset_hpa) {
    if (reading.temp_c) if (const auto o = parse_offset(temp_offset_c, 20)) *reading.temp_c += *o;
    if (reading.pressure_hpa) if (const auto o = parse_offset(pressure_offset_hpa, 50)) *reading.pressure_hpa += *o;
}
}

namespace hamclock::sensors {

QString format_temp_range(std::optional<double> low, std::optional<double> high) {
    if (!low || !high || *low > *high) return QString();
    return QString("3 h: %1 to %2 °C").arg(*low, 0, 'f', 1).arg(*high, 0, 'f', 1);
}

} // namespace hamclock::sensors
