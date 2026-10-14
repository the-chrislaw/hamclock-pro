#include "ntp_status.hpp"
#include <QProcess>
#include <QRegularExpression>
#include <cmath>

namespace hamclock::platform {

std::optional<ClockSync> parse_chrony_tracking(const QByteArray& text) {
    const QString t = QString::fromUtf8(text);
    if (!t.contains("Reference ID") || !t.contains("Leap status")) return std::nullopt;
    ClockSync s;
    const auto leap = QRegularExpression("Leap status\\s*:\\s*(.+)").match(t);
    const auto ref = QRegularExpression("Reference ID\\s*:\\s*([0-9A-Fa-f]+)").match(t);
    s.synchronized = leap.hasMatch() && !leap.captured(1).contains("Not synchronised", Qt::CaseInsensitive)
                     && ref.hasMatch() && ref.captured(1).toULongLong(nullptr, 16) != 0;
    const auto off = QRegularExpression("Last offset\\s*:\\s*([+-]?[0-9.]+(?:e[+-]?\\d+)?) seconds").match(t);
    if (off.hasMatch() && s.synchronized) {
        bool ok = false;
        const double v = off.captured(1).toDouble(&ok);
        if (ok && std::isfinite(v) && std::abs(v) < 1000.0) s.offset_ms = std::abs(v) * 1000.0;
    }
    const auto st = QRegularExpression("Stratum\\s*:\\s*(\\d+)").match(t);
    if (st.hasMatch() && s.synchronized) s.stratum = st.captured(1).toInt();
    return s;
}

std::optional<ClockSync> parse_timedatectl(const QByteArray& text) {
    const QString t = QString::fromUtf8(text).trimmed();
    if (t == "NTPSynchronized=yes") return ClockSync{true, std::nullopt, std::nullopt};
    if (t == "NTPSynchronized=no") return ClockSync{false, std::nullopt, std::nullopt};
    return std::nullopt;
}

static std::optional<QByteArray> run(const QString& program, const QStringList& args) {
    QProcess p;
    p.start(program, args, QIODevice::ReadOnly);
    if (!p.waitForStarted(500)) return std::nullopt;
    if (!p.waitForFinished(1500) || p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        p.kill();
        return std::nullopt;
    }
    return p.readAllStandardOutput();
}

std::optional<ClockSync> read_clock_sync() {
#ifdef Q_OS_LINUX
    if (const auto c = run("chronyc", {"tracking"}))
        if (const auto s = parse_chrony_tracking(*c)) return s;
    if (const auto d = run("timedatectl", {"show", "-p", "NTPSynchronized"})) return parse_timedatectl(*d);
#endif
    return std::nullopt;
}

QString describe(const ClockSync& s) {
    if (!s.synchronized) return "Not synchronised";
    QString out = "Synced";
    if (s.offset_ms) out += s.offset_ms < 1.0 ? QString(", <1 ms") : QString(", %1 ms").arg(std::lround(*s.offset_ms));
    return out;
}

} // namespace hamclock::platform
