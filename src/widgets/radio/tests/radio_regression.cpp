// rigctld read-only client against a loopback fake rigctld. Offline.
#include "../radio_model.hpp"
#include "../radio_parse.hpp"
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
    check(radio::parse_frequency_hz("14074000\n").value_or(0) == 14074000, "frequency parsed");
    check(!radio::parse_frequency_hz("RPRT -1"), "error reply is not a frequency");
    check(!radio::parse_frequency_hz("0"), "zero rejected");
    check(!radio::parse_frequency_hz("abc"), "garbage rejected");
    check(radio::parse_mode("USB").value_or("") == "USB", "mode parsed");
    check(!radio::parse_mode("RPRT -11"), "error reply is not a mode");
    check(!radio::parse_mode("a b;rm"), "odd mode rejected");
    check(radio::format_frequency(14074000) == "14.074 MHz", "format 14.074");
    check(radio::format_frequency(7030500) == "7.0305 MHz", "format 7.0305");
    check(radio::format_frequency(-1).isEmpty(), "negative format empty");
    check(radio::band_for(14074000) == "20m" && radio::band_for(145500000) == "2m", "bands");
    check(radio::band_for(15000000).isEmpty(), "outside amateur bands");

    WidgetContext ctx;
    {
        QTcpServer probe; probe.listen(QHostAddress::LocalHost, 0);
        const quint16 dead = probe.serverPort(); probe.close();
        RadioModel absent(ctx, "127.0.0.1", dead);
        absent.start();
        waitFor([&] { return false; });
        check(absent.state() == "unconfigured" && !absent.hasRig(), "no rigctld -> unconfigured");
    }
    QTcpServer server;
    check(server.listen(QHostAddress::LocalHost, 0), "fake rigctld listens");
    RadioModel model(ctx, "127.0.0.1", server.serverPort());
    model.start();
    check(waitFor([&] { return server.hasPendingConnections(); }), "client connects");
    QTcpSocket* peer = server.nextPendingConnection();
    QByteArray all;
    auto read = [&] { all += peer->readAll(); };
    check(waitFor([&] { read(); return all.contains("f\n"); }), "client asks for frequency");
    peer->write("14074000\n");
    check(waitFor([&] { read(); return all.contains("m\n"); }), "client then asks for mode");
    peer->write("USB\n2400\n");
    check(waitFor([&] { return model.state() == "live"; }), "goes live");
    check(model.frequency() == "14.074 MHz" && model.band() == "20m" && model.mode() == "USB", "values");
    // only read-only commands were ever sent
    for (const auto& line : all.split('\n')) check(line.isEmpty() || line == "f" || line == "m", "only f and m sent");
    all.clear();
    check(waitFor([&] { read(); return all.contains("f\n"); }), "polls again");
    peer->write("RPRT -1\n");
    check(waitFor([&] { return model.state() == "error"; }) && !model.hasRig(), "error reply is not live");
    peer->disconnectFromHost();
    check(waitFor([&] { return model.state() == "unconfigured"; }), "connection loss -> unconfigured");
    return failures ? 1 : 0;
}
