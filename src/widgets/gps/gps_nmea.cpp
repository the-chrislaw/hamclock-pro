#include "gps_nmea.hpp"
#include <QList>
#include <cmath>

namespace hamclock::gps {

namespace {
bool checksum_ok(const QByteArray& s, QByteArray& body) {
    const int star = s.lastIndexOf('*');
    if (!s.startsWith('$') || star < 0 || star + 3 != s.size()) return false;
    body = s.mid(1, star - 1);
    int x = 0;
    for (char c : body) x ^= static_cast<unsigned char>(c);
    bool ok = false;
    const int want = s.mid(star + 1, 2).toInt(&ok, 16);
    return ok && want == x;
}

// ddmm.mmmm + hemisphere -> signed degrees.
std::optional<double> coord(const QByteArray& v, const QByteArray& hemi, int deg_digits, double limit) {
    if (v.size() < deg_digits + 2 || hemi.size() != 1) return std::nullopt;
    bool a = false, b = false;
    const double d = v.left(deg_digits).toInt(&a);
    const double m = v.mid(deg_digits).toDouble(&b);
    if (!a || !b || m < 0 || m >= 60) return std::nullopt;
    double r = d + m / 60.0;
    const char h = hemi[0];
    if (h == 'S' || h == 'W') r = -r;
    else if (h != 'N' && h != 'E') return std::nullopt;
    return std::isfinite(r) && std::fabs(r) <= limit ? std::optional<double>(r) : std::nullopt;
}
}

Nmea parse_nmea(const QByteArray& sentence) {
    Nmea n;
    QByteArray body;
    if (!checksum_ok(sentence.trimmed(), body)) return n;
    const QList<QByteArray> f = body.split(',');
    if (f.isEmpty() || f[0].size() != 5) return n;
    const QByteArray type = f[0].mid(2);   // strip the talker (GP, GN, GL, ...)
    if (type == "GGA" && f.size() >= 10) {
        const int q = f[6].toInt();
        n.kind = Nmea::Gga;
        n.used = f[7].toInt();
        if (q <= 0) return n;
        const auto lat = coord(f[2], f[3], 2, 90), lon = coord(f[4], f[5], 3, 180);
        if (!lat || !lon) return n;
        n.fix.lat = lat; n.fix.lon = lon; n.fix.mode = 2;
        bool ok = false;
        const double alt = f[9].toDouble(&ok);
        if (ok && alt > -1000 && alt < 100000) { n.fix.alt_m = alt; n.fix.mode = 3; }
    } else if (type == "RMC" && f.size() >= 10) {
        n.kind = Nmea::Rmc;
        if (f[2] != "A") return n;
        const auto lat = coord(f[3], f[4], 2, 90), lon = coord(f[5], f[6], 3, 180);
        if (!lat || !lon) return n;
        n.fix.lat = lat; n.fix.lon = lon; n.fix.mode = 2;
        const QByteArray t = f[1], d = f[9];
        if (t.size() >= 6 && d.size() == 6) {
            const int yy = d.mid(4, 2).toInt();
            const QDateTime dt(QDate(2000 + yy, d.mid(2, 2).toInt(), d.left(2).toInt()),
                               QTime(t.left(2).toInt(), t.mid(2, 2).toInt(), t.mid(4, 2).toInt()), Qt::UTC);
            if (dt.isValid()) n.utc = dt;
        }
    }
    return n;
}

QList<QByteArray> NmeaStream::feed(const QByteArray& chunk) {
    QList<QByteArray> out;
    for (char c : chunk) {
        if (c == '\n' || c == '\r') {
            if (!overflow_ && !buf_.isEmpty()) out.append(buf_);
            buf_.clear();
            overflow_ = false;
        } else if (!overflow_) {
            buf_.append(c);
            if (buf_.size() > 128) { buf_.clear(); overflow_ = true; }
        }
    }
    return out;
}

void NmeaTracker::apply(const Nmea& n, qint64 now_ms) {
    if (n.kind == Nmea::Gga) {
        used_ = n.used;
        if (n.fix.has_position()) { fix_ = n.fix; fix_ms_ = now_ms; }
        else { fix_ = Fix{}; fix_ms_ = -1; }   // receiver reports no fix
    } else if (n.kind == Nmea::Rmc) {
        if (n.utc) { utc_ = *n.utc; utc_ms_ = now_ms; }
        if (n.fix.has_position()) {
            const auto alt = fix_.alt_m;     // RMC has no altitude: keep GGA's
            fix_ = n.fix;
            if (alt) { fix_.alt_m = alt; fix_.mode = 3; }
            fix_ms_ = now_ms;
        }
    }
}

bool NmeaTracker::fresh(qint64 now_ms) const {
    return fix_ms_ >= 0 && now_ms >= fix_ms_ && now_ms - fix_ms_ <= qint64(max_age_s_) * 1000;
}

std::optional<QDateTime> NmeaTracker::utc(qint64 now_ms) const {
    if (utc_ms_ < 0 || now_ms < utc_ms_ || now_ms - utc_ms_ > qint64(max_age_s_) * 1000) return std::nullopt;
    return utc_.addMSecs(now_ms - utc_ms_);
}

} // namespace hamclock::gps
