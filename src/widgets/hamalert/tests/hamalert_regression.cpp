// HamAlert: telnet login with password (fake socket), spot intake, unconfigured state, credential rules. Offline.
#include "../hamalert_model.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
class FakeSocket : public hamclock::TelnetSocket {
public:
    using TelnetSocket::TelnetSocket;
    void connectToHost(const QString& h, int p) override { host = h; port = p; }
    void write(const QByteArray& d) override { written.append(d); }
    void close() override {}
    QString host; int port = 0; QByteArray written;
};
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());
    FakeSocket* sock = nullptr;
    WidgetContext ctx;
    ctx.settings = &settings;
    HamAlertModel model(ctx, [&](QObject* parent) -> TelnetSocket* { return sock = new FakeSocket(parent); });

    model.start();
    check(model.state() == "unconfigured" && sock == nullptr, "no login: not configured, no connection");

    check(!settings.saveHamAlert("K1ABC", "bad pass").isEmpty(), "password with a space rejected");
    check(settings.saveHamAlert("K1ABC", "").isEmpty() && settings.hamAlertUser() == "K1ABC", "user saved");
    check(model.state() == "unconfigured", "user without password: still not configured");
    check(settings.saveHamAlert("K1ABC", "s3cret!").isEmpty() && settings.hasHamAlertPassword(), "password saved");
    check(sock && sock->host == "hamalert.org" && sock->port == 7300, "connects to hamalert.org:7300");

    emit sock->connected();
    emit sock->dataReceived("Welcome to HamAlert\r\nlogin: ");
    check(sock->written == "K1ABC\r\n", "login sent once");
    emit sock->dataReceived("Password: ");
    check(sock->written == "K1ABC\r\ns3cret!\r\n", "password sent after the prompt");
    check(model.state() == "loading", "not online before the post-login output");
    emit sock->dataReceived("\r\nHello K1ABC\r\nDX de HAMALERT: 14074.0 JA1XYZ FT8 -12 dB 1234Z\r\n");
    check(model.state() == "live" && model.count() == 1, "online, spot taken");
    const auto first = model.recent().value(0).toMap();
    check(first["dx"] == "JA1XYZ" && first["band"] == "20m", "spot fields");
    check(sock->written == "K1ABC\r\ns3cret!\r\n", "nothing else is ever sent");

    emit sock->closed("reset");
    check(model.state() == "offline", "lost connection shown as offline");

    // Without a password the client behaves as before (cluster login only).
    TelnetClient plain([&](QObject* p) -> TelnetSocket* { return sock = new FakeSocket(p); });
    plain.start("cluster.example", 7300, "K1ABC");
    emit sock->connected();
    emit sock->dataReceived("login: ");
    emit sock->dataReceived("Hello\r\n");
    check(plain.state() == TelnetClient::State::Online && sock->written == "K1ABC\r\n", "no password: unchanged flow");
    check(TelnetClient::looks_like_password_prompt("  Password: ") && !TelnetClient::looks_like_password_prompt("login:"),
          "password prompt detection");
    TelnetClient bad([&](QObject* p) -> TelnetSocket* { return sock = new FakeSocket(p); });
    bad.start("h", 7300, "K1ABC", {}, "has space");
    check(bad.state() == TelnetClient::State::Idle, "unsafe password keeps the client idle");

    // Credentials stay in the owner-only file, not options.ini.
    QFile opts(settings.optionsPath());
    check(!opts.open(QIODevice::ReadOnly) || !opts.readAll().contains("s3cret!"), "password not in options.ini");
    AppSettings reloaded(dir.filePath("station.conf"), dir.path());
    check(reloaded.hamAlertUser() == "K1ABC" && reloaded.hamAlertPassword() == "s3cret!", "login persists");
    check(reloaded.saveHamAlert("", "").isEmpty() && !reloaded.hasHamAlertPassword(), "empty user clears both");

    std::cerr << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
