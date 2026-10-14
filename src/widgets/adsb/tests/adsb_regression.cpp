// PiAware aircraft.json client against a loopback fake HTTP server. Offline.
#include "../adsb_model.hpp"
#include "../adsb_parse.hpp"
#include <QCoreApplication>
#include <QDateTime>
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
    while (!f() && t.elapsed() < 8000) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return f();
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    const auto good = adsb::parse_aircraft(R"({"now":1000.5,"messages":5,"aircraft":[
        {"hex":"a1","lat":1.0,"lon":2.0,"seen":3.0},{"hex":"a2","seen":10},{"hex":"a3","lat":1,"lon":2,"seen":300}]})");
    check(good && good->tracked == 2 && good->withPosition == 1 && good->now == 1000.5, "counts recent aircraft only");
    check(!adsb::parse_aircraft("nope") && !adsb::parse_aircraft("{}") && !adsb::parse_aircraft(R"({"now":1})"), "garbage rejected");

    WidgetContext ctx;
    {
        QTcpServer probe; probe.listen(QHostAddress::LocalHost, 0);
        const quint16 dead = probe.serverPort(); probe.close();
        AdsbModel absent(ctx, QString("http://127.0.0.1:%1/data/aircraft.json").arg(dead));
        absent.start();
        waitFor([&] { return absent.state() == "unconfigured"; });
        check(absent.state() == "unconfigured" && !absent.hasReceiver(), "no receiver -> unconfigured");
    }

    QTcpServer server;
    check(server.listen(QHostAddress::LocalHost, 0), "fake receiver listens");
    QByteArray body;
    QByteArray request;
    QObject::connect(&server, &QTcpServer::newConnection, [&] {
        auto* s = server.nextPendingConnection();
        QObject::connect(s, &QTcpSocket::readyRead, [&, s] {
            request += s->readAll();
            if (!request.contains("\r\n\r\n")) return;
            s->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: "
                     + QByteArray::number(body.size()) + "\r\n\r\n" + body);
            s->disconnectFromHost();
        });
    });
    const double nowSecs = static_cast<double>(QDateTime::currentSecsSinceEpoch());
    body = QByteArray(R"({"now":)") + QByteArray::number(nowSecs, 'f', 1)
         + R"(,"aircraft":[{"hex":"a1","lat":1,"lon":2,"seen":1},{"hex":"a2","seen":2}]})";
    AdsbModel model(ctx, QString("http://127.0.0.1:%1/data/aircraft.json").arg(server.serverPort()));
    model.start();
    check(waitFor([&] { return model.state() == "live"; }), "goes live");
    check(model.tracked() == "2" && model.withPosition() == "1", "counts shown");
    check(request.startsWith("GET /data/aircraft.json"), "only a GET of aircraft.json");

    // Receiver clock far from ours: stale, not live.
    body = QByteArray(R"({"now":1000,"aircraft":[]})");
    QMetaObject::invokeMethod(&model, "poll");
    check(waitFor([&] { return model.state() == "stale"; }), "old receiver clock -> stale");
    check(!model.hasReceiver() && model.tracked().isEmpty(), "no counts when stale");

    // Garbage: error.
    body = "<html>";
    QMetaObject::invokeMethod(&model, "poll");
    check(waitFor([&] { return model.state() == "error"; }), "garbage -> error");
    return failures ? 1 : 0;
}
