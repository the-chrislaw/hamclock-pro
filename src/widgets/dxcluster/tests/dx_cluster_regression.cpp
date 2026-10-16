// DX cluster: telnet client (fake socket), spot parser and model, offline.
#include "../dx_cluster_model.hpp"
#include "../../../map/overlay.hpp"
#include "../dx_spot_parse.hpp"
#include "../../../net/telnet_client.hpp"
#include "../../../platform/apple_lifecycle.hpp"
#include <QEventLoop>
#include <QTimer>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTimeZone>
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

static void QTest_wait_ms(int ms) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    using namespace hamclock::dxcluster;
    if (argc < 2) return 2;
    const QDateTime now(QDate(2026, 10, 13), QTime(12, 40), QTimeZone::utc());

    // Parser on the sample session.
    QFile f(QDir(argv[1]).filePath("dx-cluster-session.txt"));
    check(f.open(QIODevice::ReadOnly), "fixture opens");
    QStringList lines;
    for (const auto& l : QString::fromLatin1(f.readAll()).split('\n'))
        if (l.startsWith("SPOT|")) lines << l.mid(5);
    check(lines.size() == 3, "fixture has three spots");
    const auto a = parse_spot_line(lines[0], now);
    check(a && a->spotter == "W3LPL" && a->dx == "K1ABC" && a->frequency_khz == 14025.0, "CW spot fields");
    check(a && a->band == "20m" && a->mode == "CW" && a->grid == "FN20", "band, mode, grid");
    check(a && a->time == QDateTime(QDate(2026, 10, 13), QTime(12, 34), QTimeZone::utc()), "time is UTC today");
    const auto b = parse_spot_line(lines[1], now);
    check(b && b->spotter == "DL1XYZ-#" && b->band == "40m" && b->mode == "FT8", "skimmer spotter and FT8");
    const auto c = parse_spot_line(lines[2], now);
    check(c && c->band == "2m" && c->grid.isEmpty(), "no grid is fine");
    {
        const auto ref = [](const char* c) { const auto r = parse_ota_ref(c); return r.program + ":" + r.ref; };
        check(ref("CW POTA K-1234 up 1") == "POTA:K-1234", "POTA ref");
        check(ref("SOTA w7w/kg-001") == "SOTA:W7W/KG-001", "SOTA ref, lowercase");
        check(ref("IOTA EU-005 QRP") == "IOTA:EU-005", "IOTA ref");
        check(ref("KFF-1234 FT8") == "WWFF:KFF-1234", "WWFF ref");
        check(ref("CQ DX 599") == ":" && ref("TEST-12345X") == ":", "no false refs");
        const auto o = parse_spot_line("DX de W3LPL: 14025.0 K1ABC POTA K-1234 CW 1234Z", now);
        check(o && o->ota_program == "POTA" && o->ota_ref == "K-1234", "spot carries the reference");
    }
    check(parse_spot_line("DX de W3LPL: 14025.0 K1ABC CQ 2359Z", now)->time.date() == QDate(2026, 10, 12),
          "future time rolls back a day");
    for (const char* bad : {"", "Hello", "DX de W3LPL: abc K1ABC CQ 1234Z", "DX de W3LPL: 14025.0 K1ABC CQ 2561Z",
                            "WWV de VE7CC <12>: SFI=123", "DX de W3LPL: 0.0 K1ABC CQ 1234Z"})
        check(!parse_spot_line(bad, now), "non-spot rejected");

    // Telnet helpers.
    QByteArray pending("\xff\xfd\x01hi\xff\xfb\x03\xff\xff", 10), replies;
    const auto plain = TelnetClient::strip_negotiation(pending, replies);
    check(plain == QByteArray("hi\xff", 3) && pending.isEmpty(), "IAC stripped, escaped 0xFF kept");
    check(replies == QByteArray("\xff\xfc\x01\xff\xfe\x03", 6), "options refused");
    QByteArray partial("ab\xff", 3), r2;
    check(TelnetClient::strip_negotiation(partial, r2) == "ab" && partial.size() == 1, "partial IAC waits");
    check(TelnetClient::backoff(1).count() == 5 && TelnetClient::backoff(3).count() == 20 &&
          TelnetClient::backoff(50).count() == 300, "backoff doubles to 5 min");
    check(TelnetClient::looks_like_login_prompt("login: ") && TelnetClient::looks_like_login_prompt("Please enter your call: ") &&
          !TelnetClient::looks_like_login_prompt("Hello, welcome"), "login prompt detection");

    // Client against a fake socket.
    std::vector<FakeSocket*> sockets;
    TelnetClient client([&](QObject* parent) -> TelnetSocket* { auto* s = new FakeSocket(parent); sockets.push_back(s); return s; });
    QStringList got;
    QObject::connect(&client, &TelnetClient::lineReceived, [&](const QString& l) { got << l; });
    client.start("cluster.example", 7300, "K1ABC\r\nbye");
    check(client.state() == TelnetClient::State::Idle && sockets.empty(), "control characters in login refused");
    client.start("", 7300, "K1ABC");
    check(client.state() == TelnetClient::State::Idle && sockets.empty(), "empty host stays idle");
    client.start("cluster.example", 7300, "K1ABC");
    check(client.state() == TelnetClient::State::Connecting && sockets[0]->host == "cluster.example" && sockets[0]->port == 7300, "connecting");
    emit sockets[0]->connected();
    emit sockets[0]->dataReceived("\xff\xfd\x18Welcome\r\nlogin: ");
    check(sockets[0]->written == QByteArray("\xff\xfc\x18K1ABC\r\n", 10), "refuses option then logs in once");
    check(got == QStringList{"Welcome"} && client.state() == TelnetClient::State::LoggingIn, "greeting line delivered");
    emit sockets[0]->dataReceived("K1ABC de DB0XX\r\nDX de W3LPL: 14025.0 K1ABC CQ 1234Z\r\n");
    check(client.state() == TelnetClient::State::Online && got.size() == 3, "online after post-login output");
    emit sockets[0]->closed("reset");
    check(client.state() == TelnetClient::State::Waiting && client.consecutiveFailures() == 1, "drop schedules a retry");
    client.stop();
    check(client.state() == TelnetClient::State::Idle, "stop cancels retry");

    // App suspend: socket dropped, no retries; resume reconnects at once and resets the backoff.
    {
        client.start("cluster.example", 7300, "K1ABC");
        const auto before = sockets.size();
        auto& lifecycle = hamclock::AppLifecycle::instance();
        lifecycle.setState(Qt::ApplicationSuspended);
        check(client.state() == TelnetClient::State::Idle, "suspend closes the connection");
        QTest_wait_ms(50);
        check(sockets.size() == before, "no reconnect while suspended");
        lifecycle.setState(Qt::ApplicationActive);
        check(sockets.size() == before + 1 && client.state() == TelnetClient::State::Connecting && client.consecutiveFailures() == 0,
              "resume reconnects immediately");
        client.stop();
        lifecycle.setState(Qt::ApplicationSuspended);
        lifecycle.setState(Qt::ApplicationActive);
        check(sockets.size() == before + 1 && client.state() == TelnetClient::State::Idle, "stopped client stays stopped across suspend");
    }

    // Model: unconfigured without settings; spots flow when lines arrive.
    WidgetContext ctx;
    DxClusterModel model(ctx, [&](QObject* p) -> TelnetSocket* { return new FakeSocket(p); });
    model.setClock([&] { return now; });
    model.start();
    check(model.state() == "unconfigured", "no settings => Not configured");
    emit model.client().lineReceived(lines[0]);
    emit model.client().lineReceived("not a spot");
    check(model.spotCount() == 1, "only valid spots listed");
    {
        // Live link => layer published (the bundled DXCC table is a qrc resource, absent in unit tests, so no markers here).
        const auto& layers = MapOverlays::instance().layers();
        check(model.state() == "live" && layers.size() == 1 && layers[0].id == "dx-cluster", "live -> map layer published");
        model.setMapVisible(false);
        check(MapOverlays::instance().layers().empty(), "map off removes the layer");
        model.setMapVisible(true);
        check(MapOverlays::instance().layers().size() == 1, "map on restores it");
    }
    auto* list = qobject_cast<QAbstractListModel*>(model.spots());
    check(list && list->data(list->index(0), DxSpotList::DxRole) == "K1ABC", "newest spot first");
    check(list->data(list->index(0), DxSpotList::OtaRole).toString().isEmpty(), "no reference, no ota text");
    emit model.client().lineReceived("DX de W3LPL:     14074.0  K1POT       FT8 POTA K-1234                 1235Z FN20");
    check(list->data(list->index(0), DxSpotList::OtaRole) == "POTA K-1234", "ota role names program and reference");
    check(list->data(list->index(0), DxSpotList::NeededRole).toString().isEmpty(), "no resolver, no needed badge");
    model.setNeededResolver([](const QString& call, const QString& band) {
        return call == "K1POT" ? (band == "20m" ? QStringLiteral("NEW DXCC") : QStringLiteral("NEW BAND")) : QString();
    });
    check(list->data(list->index(0), DxSpotList::NeededRole).toString() == "NEW DXCC", "resolver gets call and band (14074 is 20m)");
    {
        auto* dl = qobject_cast<DxSpotList*>(model.spots());
        dl->set_watch({"NADXCC"});
        check(dl->watchedCount() == 1 && dl->data(dl->index(0), DxSpotList::WatchedRole).toBool(), "NADXCC keyword watches the needed-entity spot");
        dl->set_watch({"NABAND"});
        check(dl->watchedCount() == 1, "NABAND keyword also matches any needed slot");
        model.setNeededResolver([](const QString&, const QString&) { return QStringLiteral("NEW PREFIX"); });
        dl->set_watch({"NAPREF"});
        check(dl->watchedCount() == dl->totalCount(), "NAPREF matches new-prefix spots");
        dl->set_watch({"NADXCC"});
        check(dl->watchedCount() == 0, "NADXCC ignores new-prefix-only spots");
        dl->set_watch({"NAGRID"});
        check(dl->watchedCount() == 0, "NAGRID without a resolver matches nothing");
        model.setGridNeededResolver([](const QString& g) { return g == "FN20"; });
        check(dl->watchedCount() >= 1 && dl->data(dl->index(0), DxSpotList::WatchedRole).toBool(), "NAGRID matches the spot whose grid is needed");
        model.setGridNeededResolver([](const QString&) { return false; });
        check(dl->watchedCount() == 0, "NAGRID ignores spots in worked grids");
        model.setNeededResolver([](const QString&, const QString&) { return QStringLiteral("NEW BAND"); });
        dl->set_watch({"NAPREF"});
        check(dl->watchedCount() == 0, "NAPREF ignores new-band-only spots");
        dl->set_watch({"NADXCC"});
        check(dl->watchedCount() == 0, "NADXCC ignores a new-band-only spot");
        dl->set_watch({"NABAND"});
        check(dl->watchedCount() == dl->totalCount(), "resolver change re-evaluates NABAND");
        dl->set_watch({});
    }
    model.setNeededResolver({});
    {
        auto* dl = qobject_cast<DxSpotList*>(model.spots());
        dl->set_watch({"OTA"});
        check(dl->watchedCount() == 1 && dl->data(dl->index(0), DxSpotList::WatchedRole).toBool(), "OTA keyword watches spots naming a reference");
        dl->set_watch({"POTA"});
        check(dl->watchedCount() == 1, "POTA keyword matches a POTA spot");
        dl->set_watch({"SOTA", "IOTA", "WWFF"});
        check(dl->watchedCount() == 0, "other programme keywords ignore it");
        dl->set_watch({});
    }
    for (int i = 0; i < 80; ++i) emit model.client().lineReceived(lines[1]);
    check(model.spotCount() == DxSpotList::kCapacity, "list capped");

    // Filters: the list shows only matching spots; clearing restores them.
    emit model.client().lineReceived(lines[0]); // 20m CW (list is full of 40m FT8)
    check(model.spotCount() == DxSpotList::kCapacity && model.totalCount() == DxSpotList::kCapacity, "unfiltered count");
    model.setBandFilter("20m");
    check(model.spotCount() == 1 && model.totalCount() == DxSpotList::kCapacity, "band filter hides other bands");
    check(list->data(list->index(0), DxSpotList::DxRole) == "K1ABC", "filtered row is the 20m spot");
    model.setModeFilter("FT8");
    check(model.spotCount() == 0, "band and mode combine");
    emit model.client().lineReceived(lines[1]);
    check(model.spotCount() == 0, "new non-matching spot stays hidden");
    model.setBandFilter("");
    check(model.spotCount() == DxSpotList::kCapacity - 1, "mode filter alone (the CW spot is hidden)");
    model.setModeFilter("");
    check(model.spotCount() == DxSpotList::kCapacity && model.bandFilter().isEmpty(), "cleared");
    check(DxClusterModel::bandChoices().first() == "" && DxClusterModel::bandChoices().contains("20m") &&
          DxClusterModel::modeChoices().contains("FT8"), "choices offered");
    model.setBandFilter("bogus");
    check(model.bandFilter().isEmpty(), "unknown band ignored");

    // Login SSID and post-login commands.
    check(DxClusterModel::cluster_login("k1abc", 0) == "K1ABC" && DxClusterModel::cluster_login("K1ABC", 7) == "K1ABC-7" &&
          DxClusterModel::cluster_login("K1ABC", 100) == "K1ABC" && DxClusterModel::cluster_login("", 5).isEmpty(), "SSID login");
    const auto kept = TelnetClient::sanitize_commands({"set/skimmer", " sh/dx 10 ", "DX K1ABC 14025 test", "announce hi",
                                                        "set/name \x01x", "", QString(90, 'a')});
    check(kept == QStringList({"set/skimmer", "sh/dx 10"}), "only receive-only commands kept");
    check(TelnetClient::sanitize_commands(QStringList(20, "sh/dx")).size() == 8, "at most 8 commands");
    sockets.clear();
    client.start("cluster.example", 7300, "K1ABC-7", {"set/skimmer", "dx K1ABC 14025 x"});
    emit sockets[0]->connected();
    emit sockets[0]->dataReceived("login: ");
    check(sockets[0]->written == "K1ABC-7\r\n", "SSID sent as login");
    emit sockets[0]->dataReceived("Hello K1ABC-7\r\n");
    check(sockets[0]->written == "K1ABC-7\r\nset/skimmer\r\n", "commands sent once after login, spot posting refused");
    emit sockets[0]->dataReceived("more\r\n");
    check(sockets[0]->written == "K1ABC-7\r\nset/skimmer\r\n", "commands not repeated");
    client.stop();

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
