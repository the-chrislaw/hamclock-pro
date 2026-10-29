#include "radio_model.hpp"
#include "../../platform/apple_lifecycle.hpp"
#include "radio_parse.hpp"
#include "../../ui/app_settings.hpp"
#include <QSettings>

namespace hamclock {

RadioModel::RadioModel(const WidgetContext&, QString host, quint16 port, QObject* parent)
    : WidgetModel("radio", "Radio", parent), host_(std::move(host)), port_(port) {
    setState("unconfigured", "rigctld not running on this device");
    retry_.setInterval(15000);
    poll_.setInterval(1000);
    connect(&retry_, &QTimer::timeout, this, &RadioModel::connectNow);
    auto& lifecycle = AppLifecycle::instance();
    connect(&lifecycle, &AppLifecycle::suspended, this, &RadioModel::onSuspended);
    connect(&lifecycle, &AppLifecycle::resumed, this, &RadioModel::onResumed);
    connect(&poll_, &QTimer::timeout, this, &RadioModel::poll);
    connect(&flrig_, &FlrigClient::reading, this, [this](double hz, const QString& mode) {
        if (socket_.state() == QAbstractSocket::ConnectedState) return;   // rigctld wins
        fromFlrig_ = true;
        hz_ = hz; mode_ = mode;
        setState("live", mode.isEmpty() ? "Frequency only (flrig)" : "Connected (flrig)", now());
        emit changed();
    });
    connect(&flrig_, &FlrigClient::lost, this, [this] {
        if (!fromFlrig_) return;
        fromFlrig_ = false;
        hz_.reset(); mode_.clear(); passband_.reset();
        setState("unconfigured", "flrig connection lost");
        emit changed();
    });
    connect(&socket_, &QTcpSocket::connected, this, &RadioModel::onConnected);
    connect(&socket_, &QTcpSocket::readyRead, this, &RadioModel::onReadyRead);
    connect(&socket_, &QTcpSocket::disconnected, this, &RadioModel::onDisconnected);
    connect(&socket_, &QAbstractSocket::errorOccurred, this, [this] {
        if (socket_.state() == QAbstractSocket::UnconnectedState) onDisconnected();
    });
}

void RadioModel::start() { started_ = true; connectNow(); retry_.start(); flrig_.start(); }

// iPad: iOS kills sockets of suspended apps, so close cleanly and reconnect on resume.
void RadioModel::onSuspended() {
    retry_.stop();
    poll_.stop();
    flrig_.stop();
    fromFlrig_ = false;
    socket_.abort();
    if (started_) onDisconnected();
}

void RadioModel::onResumed() {
    if (!started_) return;
    connectNow();
    retry_.start();
    flrig_.start();
}

void RadioModel::connectNow() {
    if (socket_.state() != QAbstractSocket::UnconnectedState) return;
    socket_.connectToHost(host_, port_);
}

void RadioModel::onConnected() {
    buffer_.clear();
    wait_ = Wait::Idle;
    fromFlrig_ = false;
    setState("loading", "Waiting for the radio");
    poll_.start();
    poll();
}

void RadioModel::onDisconnected() {
    if (fromFlrig_) return;                  // flrig is supplying the readings
    const bool had = hz_.has_value() || state() != "unconfigured";
    poll_.stop();
    wait_ = Wait::Idle;
    hz_.reset(); mode_.clear(); passband_.reset();
    setState("unconfigured", had && state() != "unconfigured" ? "rigctld connection lost" : "rigctld not running on this device");
    emit changed();
}

void RadioModel::sendNext(const char* cmd, Wait w) {
    wait_ = w;
    socket_.write(cmd);
}

void RadioModel::poll() {
    if (socket_.state() != QAbstractSocket::ConnectedState || wait_ != Wait::Idle) return;
    sendNext("f\n", Wait::Freq);   // read-only commands only
}

void RadioModel::onReadyRead() {
    buffer_ += socket_.readAll();
    if (buffer_.size() > 4096) { buffer_.clear(); wait_ = Wait::Idle; return; }
    int nl;
    while ((nl = buffer_.indexOf('\n')) >= 0) {
        const QString line = QString::fromLatin1(buffer_.left(nl));
        buffer_.remove(0, nl + 1);
        handleLine(line);
    }
}

void RadioModel::fail(const QString& text) {
    wait_ = Wait::Idle;
    hz_.reset(); mode_.clear(); passband_.reset();
    setState("error", text);
    emit changed();
}

void RadioModel::handleLine(const QString& line) {
    switch (wait_) {
    case Wait::Freq: {
        const auto hz = radio::parse_frequency_hz(line);
        if (!hz) { fail(radio::is_error(line) ? "Radio did not report a frequency" : "Unreadable frequency from rigctld"); return; }
        hz_ = hz;
        sendNext("m\n", Wait::ModeName);
        break;
    }
    case Wait::ModeName: {
        const auto m = radio::parse_mode(line);
        mode_ = m.value_or(QString());
        passband_.reset();
        if (m) wait_ = Wait::ModePassband;     // a passband line follows a mode
        else wait_ = Wait::Idle;               // RPRT error: frequency still valid
        setState("live", mode_.isEmpty() ? "Frequency only" : "Connected", now());
        emit changed();
        break;
    }
    case Wait::ModePassband:
        wait_ = Wait::Idle;
        passband_ = radio::parse_passband_hz(line);
        emit changed();
        break;
    case Wait::Idle:
        break;
    }
}

QString RadioModel::frequency() const { return hz_ ? radio::format_frequency(*hz_) : QString(); }
QString RadioModel::band() const { return hz_ ? radio::band_for(*hz_) : QString(); }

void register_widget_radio(WidgetRegistry& registry, const WidgetContext& context) {
    radio::Endpoint ep{"127.0.0.1", 4532};
    if (context.settings) {
        QSettings ini(context.settings->optionsPath(), QSettings::IniFormat);
        ep = radio::parse_endpoint(ini.value("radio/host").toString(), ini.value("radio/port").toString(), ep);
    }
    auto* model = new RadioModel(context, ep.host, ep.port);
    // Not listed on iOS (no local station hardware there): never started.
    if (registry.add({"radio", "Radio", QUrl("qrc:/widgets/radio/RadioCard.qml"), "strip", 76}, model)) QTimer::singleShot(0, model, &RadioModel::start);
}

} // namespace hamclock
