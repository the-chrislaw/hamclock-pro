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
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

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

    {   // GPS time vs system clock
        const auto tp = gps::parse_line(R"({"class":"TPV","mode":3,"lat":1,"lon":2,"time":"2026-10-14T12:00:00.000Z"})");
        check(tp.fix.time && tp.fix.time->toString(Qt::ISODate) == "2026-10-14T12:00:00Z", "TPV time parsed");
        check(!gps::parse_line(R"({"class":"TPV","mode":3,"time":"nonsense"})").fix.time, "bad time ignored");
        check(!gps::parse_line(R"({"class":"TPV","mode":3,"time":"1970-01-01T00:00:00.000Z"})").fix.time, "implausible time ignored");
        const auto g = *tp.fix.time;
        check(gps::clock_vs_gps_text(g, g.addMSecs(900)).startsWith("Clock agrees"), "small skew is not resolved");
        check(gps::clock_vs_gps_text(g.addSecs(12), g) == "Clock is 12 s behind GPS", "behind");
        check(gps::clock_vs_gps_text(g, g.addSecs(5400)) == "Clock is 1.5 h ahead of GPS", "ahead in hours");
    }
    {   // NMEA 0183 serial sentences (reference sentences with valid checksums)
        const auto gga = gps::parse_nmea("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");
        check(gga.kind == gps::Nmea::Gga && gga.fix.mode == 3 && gga.used == 8, "GGA fix and satellites");
        check(gga.fix.lat && std::fabs(*gga.fix.lat - 48.1173) < 1e-4 && std::fabs(*gga.fix.lon - 11.5167) < 1e-4, "GGA position");
        const auto rmc = gps::parse_nmea("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A");
        check(rmc.kind == gps::Nmea::Rmc && rmc.utc && rmc.utc->toString(Qt::ISODate) == "2094-03-23T12:35:19Z" , "RMC UTC time");
        check(!gps::parse_nmea("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*48").fix.has_position(), "bad checksum rejected");
        check(!gps::parse_nmea("$GPRMC,123519,V,,,,,,,230394,,*").fix.has_position(), "void RMC has no position");
        check(gps::parse_nmea("garbage").kind == gps::Nmea::Other, "NMEA garbage ignored");

        gps::NmeaStream st;
        auto a = st.feed("$GPGGA,123519,4807.038,N,01131.0");
        check(a.isEmpty(), "partial line is held");
        a = st.feed("00,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n$GPRMC,1");
        check(a.size() == 1 && gps::parse_nmea(a[0]).kind == gps::Nmea::Gga, "line completed across chunks");
        check(st.feed(QByteArray(300, 'x') + "\n$X*00\n").size() == 1, "overlong line dropped, next line kept");

        gps::NmeaTracker tr(5);
        check(!tr.fix(0).has_position() && !tr.utc(0), "tracker empty at start");
        tr.apply(gps::parse_nmea("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47"), 1000);
        tr.apply(gps::parse_nmea("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"), 1000);
        check(tr.fix(3000).has_position() && tr.fix(3000).mode == 3 && tr.satellites() == 8, "tracker merges GGA and RMC");
        check(tr.utc(3000) && tr.utc(3000)->time().second() == 21, "UTC advances with elapsed time");
        check(!tr.fix(7000).has_position() && !tr.utc(7000), "stale fix and time are not reported");
        tr.apply(gps::parse_nmea("$GPGGA,123519,,,,,0,00,,,M,,M,,*6B"), 7500);
        check(!tr.fix(7600).has_position(), "no-fix GGA clears the position");
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

    {   // serial config validation
        using gps::SerialConfig;
        const auto d = gps::parse_serial_config("/dev/ttyUSB0", "115200", {});
        check(d.device == "/dev/ttyUSB0" && d.baud == 115200, "serial config accepted");
        check(gps::parse_serial_config("/etc/passwd", "9600", {}).device.isEmpty(), "non-tty path rejected");
        check(gps::parse_serial_config("/dev/../etc/x", "9600", {}).device.isEmpty(), "dot-dot rejected");
        check(gps::parse_serial_config("/dev/ttyS0", "123", {}).baud == 9600, "odd baud falls back");
    }
#ifdef Q_OS_LINUX
    {   // serial NMEA receiver through a pseudo-terminal
        const int master = posix_openpt(O_RDWR | O_NOCTTY);
        check(master >= 0 && grantpt(master) == 0 && unlockpt(master) == 0, "pty created");
        const char* slave = ptsname(master);
        QString dev = slave ? QString::fromLatin1(slave) : QString();
        GpsModel serial(ctx);
        serial.useSerial({dev, 9600});
        serial.start();
        check(serial.state() == "loading", "serial opens and waits for a fix");
        const QByteArray sent = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"
                                "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n";
        check(::write(master, sent.constData(), size_t(sent.size())) == sent.size(), "pty write");
        check(waitFor([&] { return serial.hasFix(); }) && serial.state() == "live", "serial fix goes live");
        check(serial.grid().startsWith("JN58") && serial.satellites() == "8 in use", "serial grid and satellites");
        ::close(master);
        check(waitFor([&] { return serial.state() == "unconfigured"; }), "serial loss clears the fix");
        GpsModel missing(ctx);
        missing.useSerial({"/dev/ttyNOPE9", 9600});
        missing.start();
        check(missing.state() == "unconfigured" && !missing.hasFix(), "absent serial device -> unconfigured");
    }
#endif
    return failures ? 1 : 0;
}
