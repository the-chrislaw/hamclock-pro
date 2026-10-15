#include "ntp_status.hpp"
#include <QtGlobal>
#ifdef Q_OS_LINUX
#include <QProcess>
#endif
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
    // "Reference ID : A29FC801 (time.example.org)": keep only host-name/address characters.
    const auto src = QRegularExpression("Reference ID\\s*:\\s*[0-9A-Fa-f]+\\s*\\(([^)]*)\\)").match(t);
    if (src.hasMatch() && s.synchronized) {
        const QString name = src.captured(1).trimmed();
        if (name.size() <= 64 && QRegularExpression("^[A-Za-z0-9.:_-]+$").match(name).hasMatch()) s.source = name;
    }
    return s;
}

std::optional<ClockSync> parse_timedatectl(const QByteArray& text) {
    const QString t = QString::fromUtf8(text).trimmed();
    if (t == "NTPSynchronized=yes") return ClockSync{true, std::nullopt, std::nullopt, {}};
    if (t == "NTPSynchronized=no") return ClockSync{false, std::nullopt, std::nullopt, {}};
    return std::nullopt;
}

std::optional<ClockSync> parse_timesync_status(const QByteArray& text) {
    const QString t = QString::fromUtf8(text);
    const auto off = QRegularExpression("Offset:\\s*([+-]?[0-9.]+)\\s*(us|ms|s)\\b").match(t);
    if (!off.hasMatch()) return std::nullopt;
    bool ok = false;
    double v = off.captured(1).toDouble(&ok);
    if (!ok || !std::isfinite(v)) return std::nullopt;
    const QString unit = off.captured(2);
    v = std::abs(v) * (unit == "us" ? 1e-3 : unit == "ms" ? 1.0 : 1000.0);
    if (v >= 1e6) return std::nullopt;
    ClockSync s;
    s.synchronized = true;
    s.offset_ms = v;
    const auto st = QRegularExpression("Stratum:\\s*(\\d+)").match(t);
    if (st.hasMatch()) s.stratum = st.captured(1).toInt();
    const auto srv = QRegularExpression("Server:\\s*\\S+\\s*\\(([^)]*)\\)").match(t);
    if (srv.hasMatch()) {
        const QString name = srv.captured(1).trimmed();
        if (name.size() <= 64 && QRegularExpression("^[A-Za-z0-9.:_-]+$").match(name).hasMatch()) s.source = name;
    }
    return s;
}

#ifdef Q_OS_LINUX
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
#endif

std::optional<ClockSync> read_clock_sync() {
#ifdef Q_OS_LINUX
    if (const auto c = run("chronyc", {"tracking"}))
        if (const auto s = parse_chrony_tracking(*c)) return s;
    if (const auto d = run("timedatectl", {"show", "-p", "NTPSynchronized"})) {
        auto s = parse_timedatectl(*d);
        if (s && s->synchronized)
            if (const auto ts = run("timedatectl", {"timesync-status"}))
                if (const auto e = parse_timesync_status(*ts)) return e;
        return s;
    }
#endif
    return std::nullopt;
}

QString describe(const ClockSync& s) {
    if (!s.synchronized) return "Not synchronised";
    QString out = "Synced";
    if (s.offset_ms) out += s.offset_ms < 1.0 ? QString(", <1 ms") : QString(", %1 ms").arg(std::lround(*s.offset_ms));
    if (s.stratum && *s.stratum > 0) out += QString(", stratum %1").arg(*s.stratum);
    if (!s.source.isEmpty()) out += " via " + s.source;
    return out;
}

} // namespace hamclock::platform
