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

} // namespace hamclock::sensors
