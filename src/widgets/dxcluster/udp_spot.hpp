#pragma once
// Spots received by UDP from logging programs (off unless the user enables it in Settings).
// Accepted datagram forms: N1MM+/DXLog-style `<spot>` XML and a plain "DX de" cluster line.
#include "dx_spot_parse.hpp"
#include <QHash>
#include <QObject>
#include <QUdpSocket>
#include <memory>

namespace hamclock::dxcluster {

// Returns nothing for anything that is not a well-formed spot. Datagrams over 4 KiB are rejected.
std::optional<DxSpot> parse_udp_spot(const QByteArray& datagram, const QDateTime& now_utc);

// WSJT-X UDP protocol (schema 2/3, big-endian QDataStream). Status messages give the dial frequency per
// client id; a Decode message "CQ [DX|POTA|...] CALL [GRID]" then becomes a spot at dial + audio offset.
// Anything truncated, oversized or with another magic number is ignored.
class WsjtxDecoder {
public:
    std::optional<DxSpot> feed(const QByteArray& datagram, const QDateTime& now_utc);
private:
    QHash<QString, quint64> dial_hz_; // by client id, bounded
};

// "By me" scope for UDP input: keep spots from this station's own programs (spotter "UDP"/"WSJT-X", i.e. no
// foreign spotter named) or whose spotter has the same base call (SSID, /P etc. ignored) as `my_call`.
// With `by_me` false everything is kept. An empty `my_call` in by-me mode keeps only the local-program spots.
bool udp_spot_in_scope(const DxSpot& spot, const QString& my_call, bool by_me);

class UdpSpotListener : public QObject {
    Q_OBJECT
public:
    explicit UdpSpotListener(QObject* parent = nullptr) : QObject(parent) {}
    // Binds 127.0.0.1 (lan = false) or all IPv4 interfaces (lan = true). port 0 stops listening.
    // Returns false when the port cannot be bound.
    bool listen(int port, bool lan);
    bool listening() const { return socket_ && socket_->state() == QAbstractSocket::BoundState; }
    int port() const { return socket_ ? socket_->localPort() : 0; }
signals:
    void spotReceived(const hamclock::dxcluster::DxSpot& spot);
private:
    void on_ready();
    std::unique_ptr<QUdpSocket> socket_;
    WsjtxDecoder wsjtx_;
};

} // namespace hamclock::dxcluster
