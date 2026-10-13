#include "telnet_client.hpp"
#include <QTcpSocket>
#include <algorithm>

namespace hamclock {
namespace {
constexpr unsigned char IAC = 255, DONT = 254, DO = 253, WONT = 252, WILL = 251, SB = 250, SE = 240;
constexpr int kMaxLineBytes = 4096;

class TcpTelnetSocket : public TelnetSocket {
public:
    explicit TcpTelnetSocket(QObject* parent) : TelnetSocket(parent), socket_(this) {
        connect(&socket_, &QTcpSocket::connected, this, &TelnetSocket::connected);
        connect(&socket_, &QTcpSocket::readyRead, this, [this] { emit dataReceived(socket_.readAll()); });
        connect(&socket_, &QTcpSocket::disconnected, this, [this] { emit closed({}); });
        connect(&socket_, &QTcpSocket::errorOccurred, this, [this] { emit closed(socket_.errorString()); });
    }
    void connectToHost(const QString& host, int port) override { socket_.connectToHost(host, static_cast<quint16>(port)); }
    void write(const QByteArray& data) override { socket_.write(data); }
    void close() override { socket_.abort(); }
private:
    QTcpSocket socket_;
};
} // namespace

TelnetSocketFactory make_tcp_socket_factory() {
    return [](QObject* parent) -> TelnetSocket* { return new TcpTelnetSocket(parent); };
}

TelnetClient::TelnetClient(TelnetSocketFactory factory, QObject* parent)
    : QObject(parent), factory_(std::move(factory)), retry_(this) {
    retry_.setSingleShot(true);
    connect(&retry_, &QTimer::timeout, this, &TelnetClient::connect_now);
}

std::chrono::seconds TelnetClient::backoff(int failures) {
    const int exponent = std::clamp(failures - 1, 0, 6);
    return std::min(std::chrono::seconds(5 << exponent), std::chrono::seconds(300));
}

// Removes complete IAC sequences from `pending`, appends our refusals to
// `replies`, and returns the plain bytes. An incomplete trailing sequence stays
// in `pending` for the next call.
QByteArray TelnetClient::strip_negotiation(QByteArray& pending, QByteArray& replies) {
    QByteArray plain;
    int i = 0;
    const int n = pending.size();
    while (i < n) {
        const auto c = static_cast<unsigned char>(pending[i]);
        if (c != IAC) { plain.append(static_cast<char>(c)); ++i; continue; }
        if (i + 1 >= n) break;
        const auto cmd = static_cast<unsigned char>(pending[i + 1]);
        if (cmd == IAC) { plain.append(static_cast<char>(IAC)); i += 2; continue; }
        if (cmd == DO || cmd == DONT || cmd == WILL || cmd == WONT) {
            if (i + 2 >= n) break;
            const char option = pending[i + 2];
            if (cmd == DO) replies.append(char(IAC)).append(char(WONT)).append(option);
            else if (cmd == WILL) replies.append(char(IAC)).append(char(DONT)).append(option);
            i += 3;
            continue;
        }
        if (cmd == SB) { // skip subnegotiation up to IAC SE
            int end = -1;
            for (int j = i + 2; j + 1 < n; ++j)
                if (static_cast<unsigned char>(pending[j]) == IAC && static_cast<unsigned char>(pending[j + 1]) == SE) { end = j + 2; break; }
            if (end < 0) break;
            i = end;
            continue;
        }
        i += 2; // other two-byte commands
    }
    pending.remove(0, i);
    return plain;
}

bool TelnetClient::looks_like_login_prompt(const QString& partial_line) {
    const QString s = partial_line.trimmed().toLower();
    return s.endsWith("login:") || s.endsWith("call:") || s.endsWith("callsign:") ||
           s.endsWith("your call:") || s.endsWith("please enter your call:");
}

void TelnetClient::start(const QString& host, int port, const QString& login, const QStringList& commands) {
    stop();
    host_ = host.trimmed();
    port_ = port;
    login_ = login.trimmed();
    commands_ = sanitize_commands(commands);
    for (const QChar c : login_) // the login goes onto the wire: no control characters or spaces
        if (!(c.isLetterOrNumber() && c.unicode() < 128) && c != '/' && c != '-') return; // stays Idle
    if (host_.isEmpty() || login_.isEmpty() || port_ <= 0 || port_ > 65535) return; // stays Idle
    failures_ = 0;
    connect_now();
}

void TelnetClient::stop() {
    retry_.stop();
    if (socket_) { socket_->disconnect(this); socket_->close(); socket_->deleteLater(); socket_ = nullptr; }
    set_state(State::Idle);
}

void TelnetClient::connect_now() {
    if (socket_) { socket_->disconnect(this); socket_->close(); socket_->deleteLater(); }
    raw_.clear(); text_.clear();
    login_sent_ = false;
    socket_ = factory_(this);
    connect(socket_, &TelnetSocket::connected, this, [this] { set_state(State::LoggingIn); });
    connect(socket_, &TelnetSocket::dataReceived, this, &TelnetClient::on_data);
    connect(socket_, &TelnetSocket::closed, this, &TelnetClient::on_closed);
    set_state(State::Connecting);
    socket_->connectToHost(host_, port_);
}

void TelnetClient::on_data(const QByteArray& data) {
    raw_.append(data);
    QByteArray replies;
    text_.append(strip_negotiation(raw_, replies));
    if (!replies.isEmpty()) socket_->write(replies);
    int newline;
    while ((newline = text_.indexOf('\n')) >= 0) {
        QByteArray line = text_.left(newline);
        text_.remove(0, newline + 1);
        if (line.endsWith('\r')) line.chop(1);
        if (state_ == State::LoggingIn && login_sent_) {
            failures_ = 0;
            set_state(State::Online);
            for (const QString& cmd : commands_) socket_->write(cmd.toLatin1() + "\r\n");
        }
        emit lineReceived(QString::fromLatin1(line));
    }
    if (text_.size() > kMaxLineBytes) text_.clear(); // hostile endless line
    if (state_ == State::LoggingIn && !login_sent_ && looks_like_login_prompt(QString::fromLatin1(text_))) {
        login_sent_ = true;
        text_.clear();
        socket_->write(login_.toLatin1() + "\r\n");
    }
}

void TelnetClient::on_closed(const QString&) {
    if (socket_) { socket_->disconnect(this); socket_->deleteLater(); socket_ = nullptr; }
    ++failures_;
    set_state(State::Waiting);
    retry_.start(static_cast<int>(std::chrono::milliseconds(backoff(failures_)).count()));
}

void TelnetClient::set_state(State state) {
    if (state == state_) return;
    state_ = state;
    emit stateChanged(state);
}

} // namespace hamclock
