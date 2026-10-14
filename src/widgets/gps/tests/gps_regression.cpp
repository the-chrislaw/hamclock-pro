// gpsd JSON parsing and the client against a loopback fake gpsd. Offline.
#include "../gps_model.hpp"
#include "../gps_nmea.hpp"
#include "../gps_parse.hpp"
#include "../../../platform/apple_lifecycle.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <cmath>
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
    const auto tpv = gps::parse_line(R"({"class":"TPV","device":"/dev/ttyACM0","mode":3,"lat":51.5072,"lon":-0.1276,"altMSL":35.2})");
    check(tpv.kind == gps::Message::Tpv && tpv.fix.has_position() && tpv.fix.alt_m.value_or(0) > 35, "3D TPV parsed");
    check(!gps::parse_line(R"({"class":"TPV","mode":1})").fix.has_position(), "no-fix TPV has no position");
    check(!gps::parse_line(R"({"class":"TPV","mode":3,"lat":95,"lon":0})").fix.has_position(), "out-of-range lat rejected");
    check(!gps::parse_line(R"({"class":"TPV","mode":3,"lat":10})").fix.has_position(), "missing lon rejected");
    check(gps::parse_line("garbage").kind == gps::Message::Other, "garbage ignored");
    check(gps::parse_line(R"({"class":"VERSION","release":"3.25"})").kind == gps::Message::Other, "other class ignored");
    const auto sky = gps::parse_line(R"({"class":"SKY","satellites":[{"PRN":1,"used":true},{"PRN":2,"used":false},{"PRN":3,"used":true}]})");
    check(sky.kind == gps::Message::SkyReport && sky.sky.used == 2 && sky.sky.seen == 3, "SKY counts used satellites");
    const auto skyc = gps::parse_line(R"({"class":"SKY","uSat":7,"nSat":11})");
    check(skyc.sky.used == 7 && skyc.sky.seen == 11, "SKY summary counts");

    {   // NMEA 0183 serial sentences (reference sentences with valid checksums)
        const auto gga = gps::parse_nmea("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");
        check(gga.kind == gps::Nmea::Gga && gga.fix.mode == 3 && gga.used == 8, "GGA fix and satellites");
        check(gga.fix.lat && std::fabs(*gga.fix.lat - 48.1173) < 1e-4 && std::fabs(*gga.fix.lon - 11.5167) < 1e-4, "GGA position");
        const auto rmc = gps::parse_nmea("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A");
        check(rmc.kind == gps::Nmea::Rmc && rmc.utc && rmc.utc->toString(Qt::ISODate) == "2094-03-23T12:35:19Z" , "RMC UTC time");
        check(!gps::parse_nmea("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*48").fix.has_position(), "bad checksum rejected");
        check(!gps::parse_nmea("$GPRMC,123519,V,,,,,,,230394,,*").fix.has_position(), "void RMC has no position");
        check(gps::parse_nmea("garbage").kind == gps::Nmea::Other, "NMEA garbage ignored");
    }

    WidgetContext ctx;
    {   // nothing listening: degrades cleanly
        QTcpServer probe; probe.listen(QHostAddress::LocalHost, 0);
        const quint16 dead = probe.serverPort(); probe.close();
        GpsModel absent(ctx, "127.0.0.1", dead);
        absent.start();
        waitFor([&] { return false; });
        check(absent.state() == "unconfigured" && !absent.hasFix() && absent.grid().isEmpty(), "no gpsd -> unconfigured");
    }
    QTcpServer server;
    check(server.listen(QHostAddress::LocalHost, 0), "fake gpsd listens");
    GpsModel model(ctx, "127.0.0.1", server.serverPort());
    model.start();
    check(waitFor([&] { return server.hasPendingConnections(); }), "client connects");
    QTcpSocket* peer = server.nextPendingConnection();
    check(waitFor([&] { return peer->bytesAvailable() > 0; }) && peer->readAll().contains("?WATCH"), "client sends WATCH");
    peer->write("{\"class\":\"TPV\",\"mode\":1}\n");
    check(waitFor([&] { return model.fixText() == "No fix"; }) && model.state() == "loading", "no fix is not live");
    peer->write("{\"class\":\"SKY\",\"uSat\":5,\"nSat\":9}\n{\"class\":\"TPV\",\"mode\":3,\"lat\":51.5072,\"lon\":-0.1276}\n");
    check(waitFor([&] { return model.hasFix(); }) && model.state() == "live", "fix goes live");
    check(model.grid() == "IO91wm" && model.position() == "51.5072°N 0.1276°W" && model.satellites() == "5 of 9", "grid, position, satellites");
    peer->disconnectFromHost();
    check(waitFor([&] { return model.state() == "unconfigured"; }) && !model.hasFix(), "connection loss clears the fix");

    // iPad suspend: the socket is closed and not retried; resume reconnects at once.
    auto& lifecycle = AppLifecycle::instance();
    while (server.hasPendingConnections()) server.nextPendingConnection();
    lifecycle.setState(Qt::ApplicationSuspended);
    waitFor([&] { return false; });
    check(!server.hasPendingConnections(), "no connection while suspended");
    lifecycle.setState(Qt::ApplicationActive);
    check(waitFor([&] { return server.hasPendingConnections(); }), "resume reconnects immediately");
    return failures ? 1 : 0;
}
