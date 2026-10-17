// Meshtastic: frame extraction, node_info decoding (hand-encoded protobuf; no real capture is reachable
// offline), the model's receive-only handshake, staleness and map publication. Offline.
#include "../mesh_model.hpp"
#include "../mesh_overlay.hpp"
#include "../../../map/overlay.hpp"
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
QByteArray varint(quint64 v) {
    QByteArray b;
    do { uchar x = v & 0x7f; v >>= 7; if (v) x |= 0x80; b.append(char(x)); } while (v);
    return b;
}
QByteArray len_field(int field, const QByteArray& body) { return varint(field << 3 | 2) + varint(body.size()) + body; }
QByteArray fixed32(int field, quint32 v) {
    QByteArray b = varint(field << 3 | 5);
    for (int i = 0; i < 4; ++i) b.append(char((v >> (8 * i)) & 0xff));
    return b;
}
QByteArray node_info(quint32 num, const QString& id, const QString& lng, const QString& shrt, double lat, double lon, quint32 heard) {
    QByteArray user = len_field(1, id.toUtf8()) + len_field(2, lng.toUtf8()) + len_field(3, shrt.toUtf8()) + varint(5 << 3) + varint(43);
    QByteArray pos = fixed32(1, quint32(qint32(std::lround(lat * 1e7)))) + fixed32(2, quint32(qint32(std::lround(lon * 1e7)))) + varint(3 << 3) + varint(12);
    QByteArray ni = varint(1 << 3) + varint(num) + len_field(2, user) + len_field(3, pos) + fixed32(5, heard);
    return varint(1 << 3) + varint(7) + len_field(4, ni); // FromRadio.id + node_info
}
QByteArray frame(const QByteArray& body) {
    QByteArray f;
    f.append(char(0x94)).append(char(0xc3)).append(char(body.size() >> 8)).append(char(body.size() & 0xff));
    return f + body;
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    check(mesh::want_config_frame(1) == QByteArray::fromHex("94c30002" "1801"), "want_config frame");
    const QByteArray a = node_info(0xa1b2c3d4, "!a1b2c3d4", "Hilltop Relay", "HTR", 49.05, -72.03, 1760000000);
    auto n = mesh::parse_from_radio(a);
    check(n && n->num == 0xa1b2c3d4 && n->short_name == "HTR" && n->long_name == "Hilltop Relay" && n->id == "!a1b2c3d4", "names decoded");
    check(n && std::fabs(n->latitude - 49.05) < 1e-6 && std::fabs(n->longitude + 72.03) < 1e-6 && n->last_heard == 1760000000, "position decoded");
    check(!mesh::parse_from_radio(node_info(5, "!5", "x", "x", 0, 0, 1)), "no position rejected");
    check(!mesh::parse_from_radio(node_info(5, "!5", "x", "x", 95, 10, 1)), "latitude over 90 rejected");
    check(!mesh::parse_from_radio(QByteArray::fromHex("08071a")), "truncated rejected");
    check(!mesh::parse_from_radio(QByteArray::fromHex("0807" "2a0100")), "other FromRadio messages ignored");

    QByteArray buf = QByteArray("junk\x94\x00", 6) + frame(a).left(10);
    check(mesh::extract_frames(buf).empty() && buf.size() == 10, "partial frame kept");
    buf.append(frame(a).mid(10) + frame(QByteArray::fromHex("0807")));
    auto fr = mesh::extract_frames(buf);
    check(fr.size() == 2 && fr[0] == a && buf.isEmpty(), "frames split after resync");
    buf = QByteArray::fromHex("94c3ffff") + frame(a);
    check(mesh::extract_frames(buf).size() == 1, "oversized length resyncs");

    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());
    FakeSocket* sock = nullptr;
    WidgetContext ctx;
    ctx.settings = &settings;
    MeshModel model(ctx, [&](QObject* parent) -> TelnetSocket* { return sock = new FakeSocket(parent); });
    const qint64 t0 = model.now().toSecsSinceEpoch();
    model.start();
    check(model.state() == "unconfigured" && !sock, "no node: not configured, no connection");
    check(!settings.saveMesh("bad host!", 4403).isEmpty() && !settings.saveMesh("n.local", 0).isEmpty() && settings.meshHost().isEmpty(), "invalid settings rejected");
    check(settings.saveStation("K1ABC", 49.0, -72.0, 0, 0, "").isEmpty() && !sock, "callsign alone does not connect");
    check(settings.saveMesh("192.168.1.50", 4403).isEmpty(), "settings saved");
    check(sock && sock->host == "192.168.1.50" && sock->port == 4403 && model.state() == "loading", "connects to the node");
    emit sock->connected();
    check(sock->written == mesh::want_config_frame(1), "only the node-list request is written");
    emit sock->dataReceived(frame(node_info(1, "!00000001", "Near", "NEAR", 49.1, -72.0, quint32(t0))) +
                            frame(node_info(2, "!00000002", "Far", "FAR", 52.0, -72.0, quint32(t0 - 100))) +
                            frame(node_info(3, "!00000003", "Old", "OLD", 49.2, -72.0, quint32(t0 - 3 * 86400))));
    check(model.state() == "live" && model.count() == 2, "fresh nodes taken, stale one dropped");
    const auto near = model.nearest();
    check(near.size() == 2 && near[0].toMap()["name"] == "NEAR" && near[0].toMap()["km"].toInt() < 20, "nearest first");
    {
        const auto& layers = MapOverlays::instance().layers();
        check(layers.size() == 1 && layers[0].id == "mesh" && layers[0].markers.size() == 2, "live nodes published on the map");
    }
    model.setMapVisible(false);
    check(MapOverlays::instance().layers().empty(), "per-source map off removes the layer");
    model.setMapVisible(true);
    check(MapOverlays::instance().layers().size() == 1, "map on restores the layer");
    {
        FakeSocket* before = sock;
        check(settings.saveMesh("192.168.1.50", 4403).isEmpty() && sock == before, "unchanged save keeps the link");
        check(settings.saveMesh("mesh.lan", 4404).isEmpty() && sock != before && sock->host == "mesh.lan" && sock->port == 4404, "changed node reconnects there");
        AppSettings again(dir.filePath("station.conf"), dir.path());
        check(again.meshHost() == "mesh.lan" && again.meshPort() == 4404, "persisted");
        emit sock->connected();
        emit sock->dataReceived(frame(node_info(1, "!00000001", "Near", "NEAR", 49.1, -72.0, quint32(t0))));
    }
    emit sock->closed("reset");
    check(model.state() == "offline" && MapOverlays::instance().layers().empty(), "lost link: offline, nothing on the map");
    check(settings.saveMesh("", 4403).isEmpty() && model.state() == "unconfigured" && model.count() == 0, "clearing the host turns it off");
    std::vector<mesh::Node> list(3);
    list[1].latitude = std::nan("");
    check(mesh::overlay_layer(list).markers.size() == 2 && mesh::overlay_layer(list, 1).markers.size() == 1, "non-finite skipped, cap applied");
    std::cerr << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
