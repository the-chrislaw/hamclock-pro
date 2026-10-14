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
        if (const auto spot = parse_udp_spot(data, QDateTime::currentDateTimeUtc())) emit spotReceived(*spot);
    }
}

} // namespace hamclock::dxcluster
