#include "rotator_model.hpp"
#include "../../platform/apple_lifecycle.hpp"
#include "rotator_parse.hpp"
#include "../../ui/app_settings.hpp"
#include <QSettings>

namespace hamclock {

RotatorModel::RotatorModel(const WidgetContext&, QString host, quint16 port, QObject* parent)
    : WidgetModel("rotator", "Rotator", parent), host_(std::move(host)), port_(port) {
    setState("unconfigured", "rotctld not running on this device");
    retry_.setInterval(15000);
    poll_.setInterval(1000);
    connect(&retry_, &QTimer::timeout, this, &RotatorModel::connectNow);
    auto& lifecycle = AppLifecycle::instance();
    connect(&lifecycle, &AppLifecycle::suspended, this, &RotatorModel::onSuspended);
    connect(&lifecycle, &AppLifecycle::resumed, this, &RotatorModel::onResumed);
    connect(&poll_, &QTimer::timeout, this, &RotatorModel::poll);
    connect(&socket_, &QTcpSocket::connected, this, &RotatorModel::onConnected);
    connect(&socket_, &QTcpSocket::readyRead, this, &RotatorModel::onReadyRead);
    connect(&socket_, &QTcpSocket::disconnected, this, &RotatorModel::onDisconnected);
    connect(&socket_, &QAbstractSocket::errorOccurred, this, [this] {
        if (socket_.state() == QAbstractSocket::UnconnectedState) onDisconnected();
    });
}

void RotatorModel::start() { started_ = true; connectNow(); retry_.start(); }

// iPad: iOS kills sockets of suspended apps, so close cleanly and reconnect on resume.
void RotatorModel::onSuspended() {
    retry_.stop();
    poll_.stop();
    socket_.abort();
    if (started_) onDisconnected();
}

void RotatorModel::onResumed() {
    if (!started_) return;
    connectNow();
    retry_.start();
}

void RotatorModel::connectNow() {
    if (socket_.state() != QAbstractSocket::UnconnectedState) return;
    socket_.connectToHost(host_, port_);
}

void RotatorModel::onConnected() {
    buffer_.clear();
    wait_ = Wait::Idle;
    setState("loading", "Waiting for the rotator");
    poll_.start();
    poll();
}

void RotatorModel::onDisconnected() {
    const bool had = az_.has_value() || state() != "unconfigured";
    poll_.stop();
    wait_ = Wait::Idle;
    az_.reset(); el_.reset();
    setState("unconfigured", had && state() != "unconfigured" ? "rotctld connection lost" : "rotctld not running on this device");
    emit changed();
}

void RotatorModel::poll() {
    if (socket_.state() != QAbstractSocket::ConnectedState || wait_ != Wait::Idle) return;
    wait_ = Wait::Azimuth;
    socket_.write("p\n");   // read-only command only
}

void RotatorModel::onReadyRead() {
    buffer_ += socket_.readAll();
    if (buffer_.size() > 4096) { buffer_.clear(); wait_ = Wait::Idle; return; }
    int nl;
    while ((nl = buffer_.indexOf('\n')) >= 0) {
        const QString line = QString::fromLatin1(buffer_.left(nl));
        buffer_.remove(0, nl + 1);
        handleLine(line);
    }
}

void RotatorModel::fail(const QString& text) {
    wait_ = Wait::Idle;
    az_.reset(); el_.reset();
    setState("error", text);
    emit changed();
}

void RotatorModel::handleLine(const QString& line) {
    switch (wait_) {
    case Wait::Azimuth: {
        const auto az = rotator::parse_azimuth(line);
        if (!az) { fail(rotator::is_error(line) ? "Rotator did not report a position" : "Unreadable position from rotctld"); return; }
        pendingAz_ = az;
        wait_ = Wait::Elevation;
        break;
    }
    case Wait::Elevation: {
        const auto el = rotator::parse_elevation(line);
        if (!el) { fail("Unreadable elevation from rotctld"); return; }
        az_ = pendingAz_; el_ = el;
        wait_ = Wait::Idle;
        setState("live", "Connected", now());
        emit changed();
        break;
    }
    case Wait::Idle:
        break;
    }
}

QString RotatorModel::azimuth() const { return az_ ? rotator::format_degrees(*az_) : QString(); }
QString RotatorModel::elevation() const { return el_ ? rotator::format_degrees(*el_) : QString(); }

void register_widget_rotator(WidgetRegistry& registry, const WidgetContext& context) {
    rotator::Endpoint ep{"127.0.0.1", 4533};
    if (context.settings) {
        QSettings ini(context.settings->optionsPath(), QSettings::IniFormat);
        ep = rotator::parse_endpoint(ini.value("rotator/host").toString(), ini.value("rotator/port").toString(), ep);
    }
    auto* model = new RotatorModel(context, ep.host, ep.port);
    // Not listed on iOS (no local station hardware there): never started.
    if (registry.add({"rotator", "Rotator", QUrl("qrc:/widgets/rotator/RotatorCard.qml"), "strip", 76}, model)) QTimer::singleShot(0, model, &RotatorModel::start);
}

} // namespace hamclock
