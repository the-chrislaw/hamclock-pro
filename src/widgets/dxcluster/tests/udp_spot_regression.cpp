// UDP spot input: datagram parser and loopback listener, offline.
#include "../udp_spot.hpp"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimeZone>
#include <QTimer>
#include <QUdpSocket>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock::dxcluster;
    const QDateTime now(QDate(2026, 10, 13), QTime(12, 40), QTimeZone::utc());

    const QByteArray xml =
        "<?xml version=\"1.0\"?><spot><app>N1MM</app><dxcall>k1abc</dxcall><frequency>14025.00</frequency>"
        "<spottercall>w3lpl</spottercall><comment>CW 599 up 1</comment><timestamp>2026-10-13 12:34:56</timestamp></spot>";
    const auto a = parse_udp_spot(xml, now);
    check(a && a->dx == "K1ABC" && a->spotter == "W3LPL" && a->frequency_khz == 14025.0, "xml fields");
    check(a && a->band == "20m" && a->mode == "CW" && a->comment == "CW 599 up 1", "band, mode, comment");
    check(a && a->time == QDateTime(QDate(2026, 10, 13), QTime(12, 34, 56), QTimeZone::utc()), "timestamp is UTC");

    const auto b = parse_udp_spot("<spot><dxcall>VP8XYZ</dxcall><frequency>7074.0</frequency></spot>", now);
    check(b && b->spotter == "UDP" && b->time == now && b->band == "40m", "missing spotter and time");
    const auto fut = parse_udp_spot("<spot><dxcall>VP8XYZ</dxcall><frequency>7074.0</frequency><timestamp>2026-10-14 12:34:56</timestamp></spot>", now);
    check(fut && fut->time == now, "future timestamp falls back to arrival time");
    const auto line = parse_udp_spot("DX de W3LPL: 14025.0 K1ABC CW 1234Z FN20", now);
    check(line && line->dx == "K1ABC" && line->grid == "FN20", "plain cluster line");

    for (const char* bad : {"", "hello", "<spot></spot>", "<spot><dxcall>K1ABC</dxcall></spot>",
                            "<spot><dxcall>K1ABC</dxcall><frequency>abc</frequency></spot>",
                            "<spot><dxcall>K1ABC</dxcall><frequency>-5</frequency></spot>",
                            "<spot><dxcall>K1 ABC</dxcall><frequency>14025</frequency></spot>",
                            "<spot><dxcall>K1ABC</dxcall><frequency>14025</frequency>",
                            "<!DOCTYPE x [<!ENTITY e \"x\">]><spot><dxcall>&e;</dxcall></spot>",
                            "<other><dxcall>K1ABC</dxcall><frequency>14025</frequency></other>"})
        check(!parse_udp_spot(bad, now), bad);
    check(!parse_udp_spot(QByteArray(5000, 'x'), now), "oversized datagram rejected");

    // Loopback: port 0 means off; a bound listener delivers a datagram.
    UdpSpotListener l;
    check(l.listen(0, false) && !l.listening(), "port 0 is off");
    check(!l.listen(70000, false), "bad port refused");
    bool bound = false;
    int port = 0;
    for (int p = 47200; p < 47230 && !bound; ++p) { bound = l.listen(p, false); port = p; }
    check(bound && l.listening() && l.port() == port, "binds a loopback port");
    int got = 0;
    QString dx;
    QObject::connect(&l, &UdpSpotListener::spotReceived, [&](const DxSpot& s) { ++got; dx = s.dx; });
    QUdpSocket tx;
    tx.writeDatagram("garbage", QHostAddress::LocalHost, quint16(port));
    tx.writeDatagram(xml, QHostAddress::LocalHost, quint16(port));
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    QObject::connect(&l, &UdpSpotListener::spotReceived, &loop, &QEventLoop::quit);
    loop.exec();
    check(got == 1 && dx == "K1ABC", "listener delivers the valid datagram only");
    l.listen(0, false);
    check(!l.listening(), "stop listening");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
