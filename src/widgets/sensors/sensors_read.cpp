#include "sensors_read.hpp"
#include <QDir>
#include <QFile>
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

} // namespace hamclock::sensors
