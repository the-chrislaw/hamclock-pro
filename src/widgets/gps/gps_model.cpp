#include "gps_model.hpp"
#include "../../platform/apple_lifecycle.hpp"
#include "../../domain/geometry.hpp"
#include "../../ui/app_settings.hpp"
#include <QSettings>
#include <cmath>

namespace hamclock {

GpsModel::GpsModel(const WidgetContext&, QString host, quint16 port, QObject* parent)
    : WidgetModel("gps", "GPS", parent), host_(std::move(host)), port_(port) {
    setState("unconfigured", "gpsd not running on this device");
    retry_.setInterval(15000);
    connect(&retry_, &QTimer::timeout, this, &GpsModel::connectNow);
    auto& lifecycle = AppLifecycle::instance();
    connect(&lifecycle, &AppLifecycle::suspended, this, &GpsModel::onSuspended);
    connect(&lifecycle, &AppLifecycle::resumed, this, &GpsModel::onResumed);
    connect(&serial_, &gps::SerialPort::data, this, &GpsModel::onSerialData);
    connect(&serial_, &gps::SerialPort::closed, this, [this] {
        const bool had = serial_seen_;
        serial_seen_ = false; fix_ = {}; haveTpv_ = false;
        setState("unconfigured", had ? "GPS serial device lost" : "GPS serial device unavailable");
        emit changed();
    });
    serial_tick_.setInterval(2000);
    connect(&serial_tick_, &QTimer::timeout, this, &GpsModel::refreshSerial);
    connect(&socket_, &QTcpSocket::connected, this, &GpsModel::onConnected);
    connect(&socket_, &QTcpSocket::readyRead, this, &GpsModel::onReadyRead);
    connect(&socket_, &QTcpSocket::disconnected, this, &GpsModel::onDisconnected);
    connect(&socket_, &QAbstractSocket::errorOccurred, this, [this] {
        if (socket_.state() == QAbstractSocket::UnconnectedState) onDisconnected();
    });
}

void GpsModel::start() {
    started_ = true;
    connectNow();
    retry_.start();
}

// iPad: iOS kills sockets of suspended apps, so close cleanly and reconnect on resume.
void GpsModel::onSuspended() {
    retry_.stop();
    serial_tick_.stop();
    serial_.close();
    socket_.abort();
    if (started_) onDisconnected();
}

void GpsModel::onResumed() {
    if (!started_) return;
    connectNow();
    retry_.start();
}

void GpsModel::connectNow() {
    if (!serial_cfg_.device.isEmpty()) {
        if (serial_.is_open()) return;
        if (serial_.open(serial_cfg_.device, serial_cfg_.baud)) {
            nmea_stream_ = {}; nmea_ = gps::NmeaTracker(); serial_seen_ = false;
            setState("loading", "Waiting for a GPS fix");
            serial_tick_.start();
        } else {
            setState("unconfigured", "GPS serial device unavailable");
        }
        return;
    }
    if (socket_.state() != QAbstractSocket::UnconnectedState) return;
    socket_.connectToHost(host_, port_);
}

void GpsModel::onConnected() {
    socket_.write("?WATCH={\"enable\":true,\"json\":true}\n");
    buffer_.clear();
    setState("loading", "Waiting for a GPS fix");
}

void GpsModel::onDisconnected() {
    const bool had = haveTpv_;
    fix_ = {}; sky_ = {}; haveTpv_ = false;
    setState("unconfigured", had ? "gpsd connection lost" : "gpsd not running on this device");
    emit changed();
}

void GpsModel::onReadyRead() {
    buffer_ += socket_.readAll();
    if (buffer_.size() > 65536) buffer_.clear();
    int nl;
    while ((nl = buffer_.indexOf('\n')) >= 0) {
        apply(buffer_.left(nl));
        buffer_.remove(0, nl + 1);
    }
}

void GpsModel::onSerialData(const QByteArray& chunk) {
    const qint64 ms = now().toMSecsSinceEpoch();
    for (const auto& line : nmea_stream_.feed(chunk)) {
        const auto n = gps::parse_nmea(line);
        if (n.kind != gps::Nmea::Other) { serial_seen_ = true; nmea_.apply(n, ms); }
    }
    refreshSerial();
}

// A position older than the tracker's limit is dropped, so a receiver that goes
// quiet or loses its fix is never shown as live.
void GpsModel::refreshSerial() {
    if (!serial_.is_open()) return;
    const qint64 ms = now().toMSecsSinceEpoch();
    fix_ = nmea_.fix(ms);
    if (fix_.has_position()) {
        if (const auto t = nmea_.utc(ms)) fix_.time = *t;
    }
    sky_ = {nmea_.satellites(), 0};   // NMEA GGA gives satellites in use only
    haveTpv_ = serial_seen_;
    if (fix_.has_position()) setState("live", fix_.mode >= 3 ? "3D fix" : "2D fix", now());
    else setState("loading", serial_seen_ ? "No GPS fix yet" : "Waiting for a GPS fix");
    emit changed();
}

void GpsModel::apply(const QByteArray& line) {
    const auto m = gps::parse_line(line);
    if (m.kind == gps::Message::Tpv) { fix_ = m.fix; haveTpv_ = true; }
    else if (m.kind == gps::Message::SkyReport) sky_ = m.sky;
    else return;
    if (!haveTpv_) return;
    if (fix_.has_position()) setState("live", fix_.mode >= 3 ? "3D fix" : "2D fix", now());
    else setState("loading", "No GPS fix yet");
    emit changed();
}

QString GpsModel::fixText() const {
    if (!haveTpv_) return QString();
    return fix_.mode >= 3 ? "3D fix" : fix_.mode == 2 ? "2D fix" : "No fix";
}

QString GpsModel::position() const {
    if (!hasFix()) return QString();
    return QString("%1°%2 %3°%4").arg(std::fabs(*fix_.lat), 0, 'f', 4).arg(*fix_.lat >= 0 ? "N" : "S")
        .arg(std::fabs(*fix_.lon), 0, 'f', 4).arg(*fix_.lon >= 0 ? "E" : "W");
}

QString GpsModel::grid() const {
    if (!hasFix()) return QString();
    return QString::fromStdString(maidenhead({*fix_.lat, *fix_.lon}, 6));
}

QString GpsModel::clockCheck() const {
    if (!hasFix() || !fix_.time) return QString();
    return gps::clock_vs_gps_text(*fix_.time, now().toUTC());
}

QString GpsModel::satellites() const {
    if (sky_.seen == 0 && sky_.used > 0) return QString("%1 in use").arg(sky_.used);
    return sky_.seen > 0 ? QString("%1 of %2").arg(sky_.used).arg(sky_.seen) : QString();
}

void register_widget_gps(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new GpsModel(context);
    if (context.settings) {
        QSettings ini(context.settings->optionsPath(), QSettings::IniFormat);
        const auto cfg = gps::parse_serial_config(ini.value("gps/device").toString(), ini.value("gps/baud").toString(), {});
        if (!cfg.device.isEmpty()) model->useSerial(cfg);
    }
    registry.add({"gps", "GPS", QUrl("qrc:/widgets/gps/GpsCard.qml"), "strip", 75}, model);
    QTimer::singleShot(0, model, &GpsModel::start);
}

} // namespace hamclock
