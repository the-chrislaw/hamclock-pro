// UDP spot input: datagram parser and loopback listener, offline.
#include "../udp_spot.hpp"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimeZone>
#include <QTimer>
#include <QNetworkProxy>
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

    {
        auto put32 = [](QByteArray& b, quint32 v) { for (int k = 3; k >= 0; --k) b.append(char((v >> (8 * k)) & 0xff)); };
        auto putstr = [&](QByteArray& b, const QByteArray& t) { put32(b, quint32(t.size())); b.append(t); };
        auto head = [&](quint32 type) { QByteArray b; put32(b, 0xadbccbdau); put32(b, 2); put32(b, type); putstr(b, "WSJT-X"); return b; };
        QByteArray st = head(1);
        for (int k = 7; k >= 0; --k) st.append(char((14074000ull >> (8 * k)) & 0xff));
        WsjtxDecoder dec;
        auto decode = [&](const QByteArray& msg, quint32 df) {
            QByteArray d = head(2);
            d.append(char(1)); put32(d, 0); put32(d, 0); d.append(QByteArray(8, '\0')); put32(d, df);
            putstr(d, "~"); putstr(d, msg); d.append(char(0));
            return d;
        };
        check(!dec.feed(decode("CQ K1ABC FN42", 1500), now), "decode before status ignored");
        check(!dec.feed(st, now), "status yields no spot");
        const auto w = dec.feed(decode("CQ DX K1ABC FN42", 1500), now);
        check(w && w->dx == "K1ABC" && w->grid == "FN42" && w->band == "20m" && w->mode == "FT8", "wsjtx CQ DX decode");
        check(w && qAbs(w->frequency_khz - 14075.5) < 1e-6, "dial plus audio offset");
        check(dec.feed(decode("CQ K1ABC", 100), now).has_value(), "plain CQ");
        check(!dec.feed(decode("K1ABC W3LPL -10", 100), now), "non-CQ ignored");
        QByteArray cut = decode("CQ K1ABC FN42", 100);
        cut.chop(20);
        check(!dec.feed(cut, now), "truncated decode ignored");
        check(!dec.feed(QByteArray("\xad\xbc\xcb\xdb\0\0\0\2\0\0\0\2", 12), now), "wrong magic ignored");
    }

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
    tx.setProxy(QNetworkProxy::NoProxy);
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
