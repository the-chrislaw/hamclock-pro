#pragma once
// Reusable line-based telnet client for DX clusters (DXSpider, AR-Cluster,
// CC-Cluster). The TCP socket is injected (TelnetSocket) so tests run offline.
// It strips telnet option negotiation (refusing every option), splits lines,
// sends the callsign once when a login prompt appears, and reconnects with
// exponential backoff.
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTimer>
#include <chrono>
#include <functional>
#include <memory>

namespace hamclock {

class TelnetSocket : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void connectToHost(const QString& host, int port) = 0;
    virtual void write(const QByteArray& data) = 0;
    virtual void close() = 0;
signals:
    void connected();
    void dataReceived(const QByteArray& data);
    void closed(const QString& error); // empty when the peer closed cleanly
};
using TelnetSocketFactory = std::function<TelnetSocket*(QObject* parent)>;
TelnetSocketFactory make_tcp_socket_factory();

class TelnetClient : public QObject {
    Q_OBJECT
public:
    enum class State { Idle, Connecting, LoggingIn, Online, Waiting };
    Q_ENUM(State)
    TelnetClient(TelnetSocketFactory factory, QObject* parent = nullptr);

    void start(const QString& host, int port, const QString& login);
    void stop();
    State state() const { return state_; }
    int consecutiveFailures() const { return failures_; }
    // Delay before reconnect attempt number `failures` (1-based): 5 s doubling to 5 min.
    static std::chrono::seconds backoff(int failures);
    // Stateless helpers, public for tests.
    static QByteArray strip_negotiation(QByteArray& pending, QByteArray& replies);
    static bool looks_like_login_prompt(const QString& partial_line);

signals:
    void stateChanged(State state);
    void lineReceived(const QString& line);

private:
    void connect_now();
    void on_data(const QByteArray& data);
    void on_closed(const QString& error);
    void set_state(State state);

    TelnetSocketFactory factory_;
    TelnetSocket* socket_ = nullptr;
    QTimer retry_;
    QString host_, login_;
    int port_ = 0;
    State state_ = State::Idle;
    QByteArray raw_;       // bytes possibly ending in a partial IAC sequence
    QByteArray text_;      // decoded bytes of the current partial line
    bool login_sent_ = false;
    int failures_ = 0;
};

} // namespace hamclock
