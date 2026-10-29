// rigctld read-only client against a loopback fake rigctld. Offline.
#include "../radio_model.hpp"
#include "../flrig_parse.hpp"
#include "../radio_parse.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
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
        const radio::Endpoint def{"127.0.0.1", 4532};
        auto e = radio::parse_endpoint("shack-pc.local", "4540", def);
        check(e.host == "shack-pc.local" && e.port == 4540, "radio endpoint from options");
        e = radio::parse_endpoint("192.168.1.5", "", def);
        check(e.host == "192.168.1.5" && e.port == 4532, "radio empty port keeps default");
        e = radio::parse_endpoint("bad host;rm", "70000", def);
        check(e.host == "127.0.0.1" && e.port == 4532, "radio bad host and port ignored");
        e = radio::parse_endpoint("", "0", def);
        check(e.host == "127.0.0.1" && e.port == 4532, "radio empty host, zero port ignored");
    }
    check(radio::parse_frequency_hz("14074000\n").value_or(0) == 14074000, "frequency parsed");
    check(!radio::parse_frequency_hz("RPRT -1"), "error reply is not a frequency");
    check(!radio::parse_frequency_hz("0"), "zero rejected");
    check(!radio::parse_frequency_hz("abc"), "garbage rejected");
    check(radio::parse_mode("USB").value_or("") == "USB", "mode parsed");
    check(radio::parse_passband_hz("2400").value_or(0) == 2400, "passband parsed");
    check(!radio::parse_passband_hz("0") && !radio::parse_passband_hz("RPRT -1") && !radio::parse_passband_hz("99999999"), "bad passband rejected");
    check(radio::format_passband(2400) == "2.4 kHz" && radio::format_passband(500) == "500 Hz" && radio::format_passband(15000) == "15 kHz", "passband text");
    check(radio::format_passband(0).isEmpty(), "no passband text for 0");
    check(!radio::parse_mode("RPRT -11"), "error reply is not a mode");
    check(!radio::parse_mode("a b;rm"), "odd mode rejected");
    check(radio::format_frequency(14074000) == "14.074 MHz", "format 14.074");
    check(radio::format_frequency(7030500) == "7.0305 MHz", "format 7.0305");
    check(radio::format_frequency(-1).isEmpty(), "negative format empty");
    check(radio::band_for(14074000) == "20m" && radio::band_for(145500000) == "2m", "bands");
    check(radio::band_for(15000000).isEmpty(), "outside amateur bands");

    check(radio::flrig_request("rig.get_vfo").contains("<methodName>rig.get_vfo</methodName>"), "flrig request");
    check(radio::flrig_frequency_hz("<?xml version=\"1.0\"?><methodResponse><params><param><value><string>7030500</string></value></param></params></methodResponse>").value_or(0) == 7030500, "flrig frequency");
    check(radio::flrig_mode("<?xml version=\"1.0\"?><methodResponse><params><param><value><string>CW</string></value></param></params></methodResponse>").value_or("") == "CW", "flrig mode");
    check(!radio::flrig_frequency_hz("<methodResponse><fault><value><string>x</string></value></fault></methodResponse>"), "flrig fault is not a frequency");
    check(!radio::flrig_frequency_hz("not xml"), "flrig garbage rejected");
    check(!radio::flrig_frequency_hz("<?xml version=\"1.0\"?><methodResponse><params><param><value><string>0</string></value></param></params></methodResponse>"), "flrig zero rejected");

    WidgetContext ctx;
    {
        // flrig client against a loopback fake HTTP server
        QTcpServer http; http.listen(QHostAddress::LocalHost, 0);
        QObject::connect(&http, &QTcpServer::newConnection, &http, [&] {
            auto* c = http.nextPendingConnection();
            QObject::connect(c, &QTcpSocket::readyRead, c, [c] {
                const QByteArray req = c->readAll();
                const bool freq = req.contains("rig.get_vfo");
                if (!freq && !req.contains("rig.get_mode")) { ++failures; std::cerr << "FAIL: unexpected flrig method\n"; }
                const QByteArray body = freq ? QByteArray("<?xml version=\"1.0\"?><methodResponse><params><param><value><string>7030500</string></value></param></params></methodResponse>") : QByteArray("<?xml version=\"1.0\"?><methodResponse><params><param><value><string>CW</string></value></param></params></methodResponse>");
                c->write("HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                c->disconnectFromHost();
            });
        });
        
        FlrigClient direct("127.0.0.1", http.serverPort());
        double got = 0; QString gotMode;
        QObject::connect(&direct, &FlrigClient::reading, [&](double hz, const QString& m) { got = hz; gotMode = m; });
        direct.start();
        check(waitFor([&] { return got > 0; }) && got == 7030500 && gotMode == "CW", "flrig client reads frequency and mode");
    }
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

    {
        // Settings change: the registered model drops the old rigctld and connects to the new address, no restart.
        QTemporaryDir dir;
        AppSettings settings({}, dir.path());
        QTcpServer first, second;
        check(first.listen(QHostAddress::LocalHost, 0) && second.listen(QHostAddress::LocalHost, 0), "two fake rigctld servers listen");
        check(settings.saveRadio("127.0.0.1", first.serverPort()).isEmpty(), "first rigctld address saved");
        WidgetContext sctx;
        sctx.settings = &settings;
        WidgetRegistry registry;
        register_widget_radio(registry, sctx);
        auto* registered = qobject_cast<WidgetModel*>(registry.model("radio"));
        check(registered != nullptr, "radio widget registered");
        check(waitFor([&] { return first.hasPendingConnections(); }), "connects to the saved rigctld address");
        QTcpSocket* old_peer = first.nextPendingConnection();
        check(settings.saveRadio("127.0.0.1", second.serverPort()).isEmpty(), "new rigctld address saved");
        check(waitFor([&] { return second.hasPendingConnections(); }), "settings change connects to the new rigctld address");
        check(waitFor([&] { return old_peer->state() != QAbstractSocket::ConnectedState; }), "old rigctld link closed");
        second.close();
        QTcpServer probe; probe.listen(QHostAddress::LocalHost, 0);
        const quint16 dead = probe.serverPort(); probe.close();
        check(settings.saveRadio("127.0.0.1", dead).isEmpty(), "unreachable rigctld address saved");
        const QString expected = QString("rigctld not reachable at 127.0.0.1:%1 — set the address in Settings").arg(dead);
        check(waitFor([&] { return registered && registered->statusText() == expected; }), "unreachable message names the address and Settings");
    }
    return failures ? 1 : 0;
}
