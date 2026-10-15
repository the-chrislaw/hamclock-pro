#include "udp_spot.hpp"
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QNetworkProxy>
#include <QTimeZone>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <cmath>

namespace hamclock::dxcluster {

namespace {
constexpr int kMaxDatagram = 4096;

bool plausible_call(const QString& c) {
    static const QRegularExpression re(R"(^[A-Z0-9/]{3,16}$)");
    return re.match(c).hasMatch();
}
} // namespace

std::optional<DxSpot> parse_udp_spot(const QByteArray& datagram, const QDateTime& now_utc) {
    if (datagram.isEmpty() || datagram.size() > kMaxDatagram) return std::nullopt;
    const QString text = QString::fromUtf8(datagram).trimmed();
    if (text.startsWith("DX de", Qt::CaseInsensitive)) return parse_spot_line(text, now_utc);
    if (!text.contains("<spot", Qt::CaseInsensitive)) return std::nullopt;

    // The XML is read as flat text elements only: no DTD or entity expansion is performed.
    QXmlStreamReader xml(text);
    QString dx, spotter, comment, freq, stamp;
    int depth = 0;
    QString current;
    while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::DTD: return std::nullopt;
        case QXmlStreamReader::StartElement:
            ++depth;
            current = xml.name().toString().toLower();
            break;
        case QXmlStreamReader::EndElement: --depth; current.clear(); break;
        case QXmlStreamReader::Characters:
            if (depth == 2 && !xml.isWhitespace()) {
                const QString t = xml.text().toString().trimmed();
                if (current == "dxcall") dx = t;
                else if (current == "spottercall" || current == "spotter") spotter = t;
                else if (current == "frequency") freq = t;
                else if (current == "comment") comment = t.left(120);
                else if (current == "timestamp") stamp = t;
            }
            break;
        default: break;
        }
    }
    if (xml.hasError()) return std::nullopt;
    dx = dx.toUpper();
    spotter = spotter.toUpper();
    if (!plausible_call(dx) || (!spotter.isEmpty() && !plausible_call(spotter))) return std::nullopt;
    bool ok = false;
    const double khz = freq.toDouble(&ok); // N1MM sends kHz
    if (!ok || !std::isfinite(khz) || khz <= 0 || khz > 1.0e7) return std::nullopt;

    DxSpot spot;
    spot.dx = dx;
    spot.spotter = spotter.isEmpty() ? QStringLiteral("UDP") : spotter;
    spot.frequency_khz = khz;
    spot.comment = comment;
    spot.band = band_for_khz(khz);
    QDateTime t = QDateTime::fromString(stamp, "yyyy-MM-dd hh:mm:ss");
    t.setTimeZone(QTimeZone::utc());
    // A missing or implausible timestamp (more than 5 minutes ahead) falls back to arrival time.
    spot.time = (t.isValid() && t <= now_utc.addSecs(300)) ? t : now_utc;
    static const QRegularExpression mode_re(R"(\b(CW|SSB|FT8|FT4|RTTY|PSK\d*)\b)", QRegularExpression::CaseInsensitiveOption);
    const auto mm = mode_re.match(spot.comment);
    if (mm.hasMatch()) spot.mode = mm.captured(1).toUpper();
    return spot;
}

namespace {
// Bounds-checked big-endian reader; every read fails (and stays failed) past the end.
struct Reader {
    const QByteArray& d;
    int pos = 0;
    bool ok = true;
    quint64 num(int bytes) {
        if (!ok || pos + bytes > d.size()) { ok = false; return 0; }
        quint64 v = 0;
        for (int i = 0; i < bytes; ++i) v = (v << 8) | quint8(d[pos++]);
        return v;
    }
    QString str() {
        const quint32 len = quint32(num(4));
        if (len == 0xffffffffu) return {};
        if (!ok || len > 512 || pos + int(len) > d.size()) { ok = false; return {}; }
        const QString s = QString::fromUtf8(d.constData() + pos, int(len));
        pos += int(len);
        return s;
    }
};
} // namespace

std::optional<DxSpot> WsjtxDecoder::feed(const QByteArray& datagram, const QDateTime& now_utc) {
    if (datagram.size() < 12 || datagram.size() > kMaxDatagram) return std::nullopt;
    Reader r{datagram};
    if (r.num(4) != 0xadbccbdau) return std::nullopt;
    const quint32 schema = quint32(r.num(4));
    if (schema < 2 || schema > 3) return std::nullopt;
    const quint32 type = quint32(r.num(4));
    const QString id = r.str();
    if (!r.ok) return std::nullopt;
    if (type == 1) { // Status
        const quint64 dial = r.num(8);
        if (!r.ok || dial == 0 || dial > 10'000'000'000ull) return std::nullopt;
        if (dial_hz_.size() >= 16 && !dial_hz_.contains(id)) dial_hz_.clear();
        dial_hz_.insert(id, dial);
        return std::nullopt;
    }
    if (type != 2 || !dial_hz_.contains(id)) return std::nullopt; // Decode needs a known dial frequency
    r.num(1);                      // new
    r.num(4);                      // time of day
    r.num(4);                      // snr
    r.num(8);                      // dt
    const quint32 df = quint32(r.num(4));
    const QString mode = r.str();
    const QString message = r.str().trimmed().toUpper();
    if (!r.ok || df > 5000) return std::nullopt;
    const QStringList w = message.split(' ', Qt::SkipEmptyParts);
    if (w.size() < 2 || w[0] != "CQ") return std::nullopt;
    int i = 1;
    if (w.size() > i + 1 && w[i].size() <= 4 && !w[i].contains(QRegularExpression("\\d"))) ++i; // CQ DX / CQ POTA
    const QString call = w[i];
    if (!plausible_call(call)) return std::nullopt;
    DxSpot spot;
    spot.dx = call;
    spot.spotter = "WSJT-X";
    spot.frequency_khz = (double(dial_hz_.value(id)) + df) / 1000.0;
    spot.band = band_for_khz(spot.frequency_khz);
    spot.mode = mode == "~" ? "FT8" : mode == "+" ? "FT4" : QString();
    static const QRegularExpression grid_re("^[A-R]{2}\\d{2}$");
    if (w.size() > i + 1 && grid_re.match(w[i + 1]).hasMatch()) spot.grid = w[i + 1];
    spot.comment = message.left(40);
    spot.time = now_utc;
    return spot;
}

bool UdpSpotListener::listen(int port, bool lan) {
    socket_.reset();
    if (port < 1 || port > 65535) return port == 0;
    auto s = std::make_unique<QUdpSocket>();
    s->setProxy(QNetworkProxy::NoProxy); // local socket: never consult a system proxy
    if (!s->bind(lan ? QHostAddress(QHostAddress::AnyIPv4) : QHostAddress(QHostAddress::LocalHost), quint16(port),
                 QAbstractSocket::DontShareAddress))
        return false;
    connect(s.get(), &QUdpSocket::readyRead, this, &UdpSpotListener::on_ready);
    socket_ = std::move(s);
    return true;
}

void UdpSpotListener::on_ready() {
    while (socket_ && socket_->hasPendingDatagrams()) {
        const QByteArray data = socket_->receiveDatagram(kMaxDatagram + 1).data();
        const QDateTime now = QDateTime::currentDateTimeUtc();
        if (const auto spot = parse_udp_spot(data, now)) emit spotReceived(*spot);
        else if (const auto w = wsjtx_.feed(data, now)) emit spotReceived(*w);
    }
}

} // namespace hamclock::dxcluster
