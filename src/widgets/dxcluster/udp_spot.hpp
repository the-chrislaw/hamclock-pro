#pragma once
// Spots received by UDP from logging programs (off unless the user enables it in Settings).
// Accepted datagram forms: N1MM+/DXLog-style `<spot>` XML and a plain "DX de" cluster line.
#include "dx_spot_parse.hpp"
#include <QObject>
#include <QUdpSocket>
#include <memory>

namespace hamclock::dxcluster {

// Returns nothing for anything that is not a well-formed spot. Datagrams over 4 KiB are rejected.
std::optional<DxSpot> parse_udp_spot(const QByteArray& datagram, const QDateTime& now_utc);

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
};

} // namespace hamclock::dxcluster
