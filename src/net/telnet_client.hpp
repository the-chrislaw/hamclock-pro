#pragma once
// Reusable line-based telnet client for DX clusters (DXSpider, AR-Cluster,
// CC-Cluster). The TCP socket is injected (TelnetSocket) so tests run offline.
// It strips telnet option negotiation (refusing every option), splits lines,
// sends the callsign once when a login prompt appears, and reconnects with
// exponential backoff. While the app is suspended (AppLifecycle) it closes the
// socket and stays quiet, then reconnects at once on resume.
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <algorithm>
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

    // `commands` are sent one per line, once, right after the cluster's first post-login output.
    // A non-empty `password` (printable ASCII, no spaces) is sent once, when a "password:" prompt follows the
    // login; the client counts as online only after that. It is never logged or exposed.
    void start(const QString& host, int port, const QString& login, const QStringList& commands = {},
               const QString& password = {});
    void stop();
    State state() const { return state_; }
    int consecutiveFailures() const { return failures_; }
    // Delay before reconnect attempt number `failures` (1-based): 5 s doubling to 5 min.
    static std::chrono::seconds backoff(int failures);
    // Stateless helpers, public for tests.
    static QByteArray strip_negotiation(QByteArray& pending, QByteArray& replies);
    static bool looks_like_login_prompt(const QString& partial_line);
    static bool looks_like_password_prompt(const QString& partial_line);
    // Keeps the commands that are safe to send unattended: printable ASCII, <= 80 chars, at most 8, and
    // none that publish to the cluster (dx, announce, talk, say, ...); this client is receive-only.
    static QStringList sanitize_commands(const QStringList& commands);

signals:
    void stateChanged(State state);
    void lineReceived(const QString& line);

private:
    void connect_now();
    void on_suspended();
    void on_resumed();
    void drop_socket();
    void on_data(const QByteArray& data);
    void on_closed(const QString& error);
    void set_state(State state);

    TelnetSocketFactory factory_;
    TelnetSocket* socket_ = nullptr;
    QTimer retry_;
    QString host_, login_, password_;
    QStringList commands_;
    int port_ = 0;
    State state_ = State::Idle;
    QByteArray raw_;       // bytes possibly ending in a partial IAC sequence
    QByteArray text_;      // decoded bytes of the current partial line
    bool login_sent_ = false;
    bool password_sent_ = false;
    int failures_ = 0;
    bool wanted_ = false;    // start() accepted the settings and stop() has not been called
    bool suspended_ = false; // app suspended (iPadOS drops sockets); reconnect on resume
};

// Header-only so settings code can filter without linking the client.
inline QStringList TelnetClient::sanitize_commands(const QStringList& commands) {
    static const QStringList publishing{"dx", "announce", "ann", "talk", "say", "shout", "send", "reply", "wx", "wcy", "wwv"};
    QStringList out;
    for (const QString& raw : commands) {
        const QString cmd = raw.trimmed();
        if (cmd.isEmpty() || cmd.size() > 80 || out.size() >= 8) continue;
        const bool printable = std::all_of(cmd.begin(), cmd.end(), [](QChar c) { return c.unicode() >= 32 && c.unicode() < 127; });
        const QString verb = cmd.section(' ', 0, 0).toLower();
        const QString root = verb.section('/', 0, 0);
        if (!printable || publishing.contains(root)) continue;
        out << cmd;
    }
    return out;
}

} // namespace hamclock
