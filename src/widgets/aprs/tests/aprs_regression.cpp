// APRS: packet parsing (plain, compressed, ambiguity, rejects) and the model's receive-only login. Offline.
#include "../aprs_model.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
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
    auto p = aprs::parse_packet("N0CALL-9>APRS,TCPIP*:!4903.50N/07201.75W-Test comment");
    check(p && p->call == "N0CALL-9" && std::fabs(p->latitude - 49.0583) < 1e-3 && std::fabs(p->longitude + 72.0292) < 1e-3, "plain position");
    check(p && p->symbol == "/-" && p->comment == "Test comment", "symbol and comment");
    p = aprs::parse_packet("K1ABC>APRS:@092345z4903.50N/07201.75W>PHG");
    check(p && std::fabs(p->latitude - 49.0583) < 1e-3, "timestamped position");
    p = aprs::parse_packet("K1ABC>APRS:=4903.  N/07201.  W-");
    check(p && std::fabs(p->latitude - 49.05) < 1e-6, "ambiguity spaces count as zero");
    p = aprs::parse_packet("K1ABC>APRS:!/5L!!<*e7>{?!");
    check(p && std::fabs(p->latitude - 49.5) < 0.01 && std::fabs(p->longitude + 72.75) < 0.01, "compressed position (spec example)");
    check(!aprs::parse_packet("# aprsc 2.1 keepalive"), "server comment ignored");
    check(!aprs::parse_packet("K1ABC>APRS:>status text"), "status ignored");
    check(!aprs::parse_packet("K1ABC>APRS:!9903.50N/07201.75W-"), "latitude over 90 rejected");
    check(!aprs::parse_packet("bad call>APRS:!4903.50N/07201.75W-"), "bad call rejected");
    check(!aprs::parse_packet("K1ABC>APRS:!4903.50N/07201"), "truncated rejected");

    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());
    FakeSocket* sock = nullptr;
    WidgetContext ctx;
    ctx.settings = &settings;
    AprsModel model(ctx, [&](QObject* parent) -> TelnetSocket* { return sock = new FakeSocket(parent); });
    model.start();
    check(model.state() == "unconfigured" && !sock, "no callsign: not configured, no connection");

    check(settings.saveStation("K1ABC", 49.0, -72.0, 0, 0, "").isEmpty(), "station saved");
    check(sock && sock->host == "rotate.aprs2.net" && sock->port == 14580, "connects to APRS-IS");
    emit sock->connected();
    check(sock->written == "user K1ABC pass -1 vers hamclock-pro 0.1 filter r/49.00/-72.00/100\r\n", "receive-only login");
    emit sock->dataReceived("# aprsc 2.1\r\nN0CALL-9>APRS:!4903.50N/07201.75W-near\r\nFAR>APRS:!5003.50N/07201.75W-far\r\n");
    check(model.state() == "live" && model.count() == 2, "stations taken");
    const auto n = model.nearest();
    check(n.size() == 2 && n[0].toMap()["call"] == "N0CALL-9" && n[0].toMap()["km"].toInt() < 20, "nearest first");
    emit sock->closed("reset");
    check(model.state() == "offline", "lost connection shown as offline");
    std::cerr << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
