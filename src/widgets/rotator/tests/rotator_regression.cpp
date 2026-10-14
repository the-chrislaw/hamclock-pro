// rotctld read-only client against a loopback fake rotctld. Offline.
#include "../rotator_model.hpp"
#include "../rotator_parse.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
template <class F> bool waitFor(F f) {
    QElapsedTimer t; t.start();
    while (!f() && t.elapsed() < 5000) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return f();
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    {
        const rotator::Endpoint def{"127.0.0.1", 4533};
        auto e = rotator::parse_endpoint("shack-pc.local", "4540", def);
        check(e.host == "shack-pc.local" && e.port == 4540, "rotator endpoint from options");
        e = rotator::parse_endpoint("192.168.1.5", "", def);
        check(e.host == "192.168.1.5" && e.port == 4533, "rotator empty port keeps default");
        e = rotator::parse_endpoint("bad host;rm", "70000", def);
        check(e.host == "127.0.0.1" && e.port == 4533, "rotator bad host and port ignored");
        e = rotator::parse_endpoint("", "0", def);
        check(e.host == "127.0.0.1" && e.port == 4533, "rotator empty host, zero port ignored");
    }
    check(rotator::parse_azimuth("123.5\n").value_or(-1) == 123.5, "azimuth parsed");
    check(!rotator::parse_azimuth("RPRT -1") && rotator::is_error("RPRT -1"), "error reply is not an azimuth");
    check(!rotator::parse_azimuth("400"), "azimuth out of range");
    check(!rotator::parse_azimuth("abc"), "garbage rejected");
    check(rotator::parse_elevation("-5").value_or(0) == -5, "negative elevation");
    check(!rotator::parse_elevation("95"), "elevation out of range");
    check(rotator::format_degrees(123.5) == QString::fromUtf8("124°"), "format");
    check(rotator::format_degrees(NAN).isEmpty(), "nan format empty");

    WidgetContext ctx;
    {
        QTcpServer probe; probe.listen(QHostAddress::LocalHost, 0);
        const quint16 dead = probe.serverPort(); probe.close();
        RotatorModel absent(ctx, "127.0.0.1", dead);
        absent.start();
        waitFor([&] { return false; });
        check(absent.state() == "unconfigured" && !absent.hasRotator(), "no rotctld -> unconfigured");
    }
    QTcpServer server;
    check(server.listen(QHostAddress::LocalHost, 0), "fake rotctld listens");
    RotatorModel model(ctx, "127.0.0.1", server.serverPort());
    model.start();
    check(waitFor([&] { return server.hasPendingConnections(); }), "client connects");
    QTcpSocket* peer = server.nextPendingConnection();
    QByteArray all;
    auto read = [&] { all += peer->readAll(); };
    check(waitFor([&] { read(); return all.contains("p\n"); }), "client asks for position");
    peer->write("123.5\n45.0\n");
    check(waitFor([&] { return model.state() == "live"; }), "goes live");
    check(model.azimuth() == QString::fromUtf8("124°") && model.elevation() == QString::fromUtf8("45°"), "values");

    // Error reply: never live, no stale values.
    all.clear();
    check(waitFor([&] { read(); return all.contains("p\n"); }), "client polls again");
    peer->write("RPRT -1\n");
    check(waitFor([&] { return model.state() == "error"; }), "error reply -> error state");
    check(!model.hasRotator() && model.azimuth().isEmpty(), "no stale position after error");

    // Connection loss.
    peer->disconnectFromHost();
    check(waitFor([&] { return model.state() == "unconfigured"; }), "lost connection -> unconfigured");

    // Only the read-only command was ever sent.
    all.replace("p\n", "");
    check(all.isEmpty(), "nothing but 'p' was sent");

    if (failures == 0) std::cout << "rotator tests passed\n";
    return failures == 0 ? 0 : 1;
}
