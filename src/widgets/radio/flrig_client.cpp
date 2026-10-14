#include "flrig_client.hpp"
#include "flrig_parse.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace hamclock {

FlrigClient::FlrigClient(QString host, quint16 port, QObject* parent)
    : QObject(parent), host_(std::move(host)), port_(port) {
    connect(&timer_, &QTimer::timeout, this, &FlrigClient::poll);
}

void FlrigClient::start() { timer_.setInterval(up_ ? 1000 : 15000); timer_.start(); poll(); }
void FlrigClient::stop() { timer_.stop(); }

void FlrigClient::fail() {
    busy_ = false;
    timer_.setInterval(15000);
    if (up_) { up_ = false; emit lost(); }
}

void FlrigClient::poll() {
    if (busy_) return;
    busy_ = true;
    QNetworkRequest req(QUrl(QString("http://%1:%2/RPC2").arg(host_).arg(port_)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
    req.setHeader(QNetworkRequest::UserAgentHeader, "HamClockPro (read-only flrig client)");
    req.setTransferTimeout(3000);
    auto* freq = net_.post(req, radio::flrig_request("rig.get_vfo"));
    connect(freq, &QNetworkReply::finished, this, [this, freq, req] {
        freq->deleteLater();
        const auto hz = freq->error() == QNetworkReply::NoError ? radio::flrig_frequency_hz(freq->readAll()) : std::nullopt;
        if (!hz) { fail(); return; }
        auto* mode = net_.post(req, radio::flrig_request("rig.get_mode"));
        connect(mode, &QNetworkReply::finished, this, [this, mode, hz] {
            mode->deleteLater();
            const auto m = mode->error() == QNetworkReply::NoError ? radio::flrig_mode(mode->readAll()) : std::nullopt;
            busy_ = false;
            if (!up_) { up_ = true; timer_.setInterval(1000); }
            emit reading(*hz, m.value_or(QString()));
        });
    });
}

} // namespace hamclock
