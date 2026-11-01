// DX cluster setup: suggested public nodes and the status text a new user sees
// (not configured / connecting / cannot reach / live / connection lost). Fake socket, offline.
#include "../cluster_nodes.hpp"
#include "../dx_cluster_model.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
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
    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());

    // Every suggestion is accepted by the same validation as a typed host, and offered to QML.
    for (const auto& node : dxcluster::kSuggestedNodes)
        check(settings.saveCluster(node.host, node.port).isEmpty(), node.host);
    const auto offered = settings.clusterSuggestions();
    check(offered.size() == int(dxcluster::kSuggestedNodes.size()), "suggestions offered to Settings");
    const auto first = offered.value(0).toMap();
    check(first["host"] == "dxc.nc7j.com" && first["port"] == 7373 && !first["label"].toString().isEmpty() &&
              !first["note"].toString().isEmpty(),
          "suggestion fields");
    check(settings.saveCluster("", 7300).isEmpty() && settings.clusterHost().isEmpty(), "out of the box: no host");

    std::vector<FakeSocket*> sockets;
    WidgetContext ctx;
    ctx.settings = &settings;
    DxClusterModel model(ctx, [&](QObject* p) -> TelnetSocket* { auto* s = new FakeSocket(p); sockets.push_back(s); return s; });
    model.start();
    check(model.state() == "unconfigured" && model.statusText().contains("set a cluster host") && sockets.empty(),
          "no host: not configured, never connects");

    check(settings.saveCluster("dxc.nc7j.com", 7373).isEmpty(), "pick a node");
    check(model.state() == "unconfigured" && model.statusText().contains("callsign") && sockets.empty(),
          "host but no callsign: asks for the callsign, never connects");

    check(settings.saveStation("W1AW", 41.714, -72.727, 0, 0, "").isEmpty(), "callsign set");
    check(!sockets.empty() && sockets.back()->host == "dxc.nc7j.com" && sockets.back()->port == 7373, "connects to the chosen node");
    check(model.state() == "loading", "connecting");

    // The node never answers: say so, with the address, instead of "connection lost".
    emit sockets.back()->closed("Host not found");
    check(model.state() == "offline" && model.statusText().contains("Cannot reach dxc.nc7j.com:7373"),
          "unreachable node named");

    model.start();
    auto* sock = sockets.back();
    emit sock->connected();
    emit sock->dataReceived("Welcome to NC7J, running AR-Cluster\r\nlogin: ");
    check(sock->written == "W1AW\r\n", "logs in with the callsign");
    emit sock->dataReceived("\r\nHello W1AW\r\n");
    check(model.state() == "live" && model.statusText() == "Cluster dxc.nc7j.com", "live");
    emit sock->closed("reset");
    check(model.state() == "offline" && model.statusText().contains("connection lost"), "drop after live: connection lost");

    // Choosing another node in Settings reconnects there without a restart; unrelated changes do not.
    const auto before = sockets.size();
    check(settings.saveWatchList("K1ABC").isEmpty() && sockets.size() == before, "other options leave the link alone");
    check(settings.saveCluster("k1ttt.net", 7373).isEmpty() && sockets.size() == before + 1 &&
              sockets.back()->host == "k1ttt.net",
          "new node: reconnects at once");
    check(settings.saveClusterLogin(2, "").isEmpty() && sockets.size() == before + 2, "new SSID: reconnects");
    emit sockets.back()->connected();
    emit sockets.back()->dataReceived("Please enter your call: ");
    check(sockets.back()->written == "W1AW-2\r\n", "logs in with the SSID");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
