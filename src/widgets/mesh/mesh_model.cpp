#include "mesh_model.hpp"
#include "../dxcluster/spot_layer_style.hpp"
#include "mesh_overlay.hpp"
#include "../../map/overlay.hpp"
#include "../../domain/geometry.hpp"
#include "../../ui/app_settings.hpp"
#include <algorithm>

namespace hamclock {

MeshModel::MeshModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent)
    : WidgetModel("mesh", "Meshtastic", parent), settings_(context.settings), factory_(std::move(factory)) {
    if (settings_) {
        map_visible_ = settings_->mapSourceVisible("mesh");
        connect(settings_, &AppSettings::optionsChanged, this, [this] {
            const bool v = settings_->mapSourceVisible("mesh");
            if (v != map_visible_) setMapVisible(v);
            else publishOverlay();
        });
        connect(settings_, &AppSettings::meshChanged, this, &MeshModel::start);
    }
    retry_.setSingleShot(true);
    connect(&retry_, &QTimer::timeout, this, &MeshModel::connect_now);
}

MeshModel::~MeshModel() { MapOverlays::instance().remove_layer(mesh::kOverlayLayerId); }

void MeshModel::setMapVisible(bool visible) {
    map_visible_ = visible;
    publishOverlay();
}

// Only while the node link is live; a lost link never leaves nodes on the map.
void MeshModel::publishOverlay() {
    if (!map_visible_ || state() != "live") { MapOverlays::instance().remove_layer(mesh::kOverlayLayerId); return; }
    std::vector<mesh::Node> list;
    for (const auto& n : nodes_) list.push_back(n);
    std::sort(list.begin(), list.end(), [](const mesh::Node& a, const mesh::Node& b) { return a.last_heard > b.last_heard; });
    auto layer = mesh::overlay_layer(list);
    apply_spot_label_style(layer, settings_, "mesh");
    MapOverlays::instance().set_layer(layer);
}

void MeshModel::start() {
    wanted_ = false;
    retry_.stop();
    if (socket_) { socket_->close(); socket_->deleteLater(); socket_ = nullptr; }
    nodes_.clear();
    partial_.clear();
    emit changed();
    if (!settings_ || settings_->meshHost().isEmpty()) {
        setState("unconfigured", "Not configured: enter your Meshtastic node in Settings");
        publishOverlay();
        return;
    }
    setState("loading", "Connecting to the Meshtastic node…");
    publishOverlay();
    wanted_ = true;
    failures_ = 0;
    connect_now();
}

void MeshModel::connect_now() {
    if (!wanted_) return;
    if (socket_) { socket_->close(); socket_->deleteLater(); }
    socket_ = factory_(this);
    partial_.clear();
    setState("loading", "Connecting to the Meshtastic node…");
    connect(socket_, &TelnetSocket::connected, this, [this] { socket_->write(mesh::want_config_frame(1)); });
    connect(socket_, &TelnetSocket::dataReceived, this, &MeshModel::on_data);
    connect(socket_, &TelnetSocket::closed, this, [this](const QString&) { on_closed(); });
    socket_->connectToHost(settings_->meshHost(), settings_->meshPort());
}

void MeshModel::on_closed() {
    if (!wanted_) return;
    ++failures_;
    setState("offline", "Meshtastic node connection lost; retrying");
    publishOverlay();
    retry_.start(int(TelnetClient::backoff(failures_).count()) * 1000);
}

void MeshModel::on_data(const QByteArray& data) {
    partial_.append(data);
    bool any = false;
    for (const QByteArray& frame : mesh::extract_frames(partial_)) {
        failures_ = 0;
        setState("live", "Meshtastic node", now()); // any well-formed frame proves the node is talking
        if (const auto u = mesh::parse_position_packet(frame)) {
            // A live position report moves a known node, or adds one we have no name for yet.
            const qint64 t = u->rx_time > 0 ? u->rx_time : now().toSecsSinceEpoch();
            auto it = nodes_.find(u->num);
            if (it == nodes_.end()) {
                if (int(nodes_.size()) >= kCapacity) continue;
                mesh::Node fresh;
                fresh.num = u->num;
                fresh.id = QString("!%1").arg(u->num, 8, 16, QLatin1Char('0'));
                it = nodes_.insert(u->num, fresh);
            }
            it->latitude = u->latitude;
            it->longitude = u->longitude;
            it->last_heard = t;
            any = true;
            continue;
        }
        if (const auto nm = mesh::parse_nodeinfo_packet(frame)) {
            // A renamed node keeps its position; names for nodes we have no position for are not kept.
            auto it = nodes_.find(nm->num);
            if (it == nodes_.end()) continue;
            if (!nm->long_name.isEmpty()) it->long_name = nm->long_name;
            if (!nm->short_name.isEmpty()) it->short_name = nm->short_name;
            any = true;
            continue;
        }
        const auto n = mesh::parse_from_radio(frame);
        if (!n) continue;
        if (n->last_heard > 0 && now().toSecsSinceEpoch() - n->last_heard > kMaxAgeSec) { nodes_.remove(n->num); continue; }
        if (!nodes_.contains(n->num) && int(nodes_.size()) >= kCapacity) continue;
        nodes_.insert(n->num, *n);
        any = true;
    }
    if (any) emit changed();
    publishOverlay();
}

QVariantList MeshModel::nearest() const {
    struct Row { double km; const mesh::Node* n; };
    std::vector<Row> rows;
    const Location de{settings_ ? settings_->deLatitude() : 0, settings_ ? settings_->deLongitude() : 0};
    for (const auto& n : nodes_) {
        double km = 0;
        try { km = geometry(de, {n.latitude, n.longitude}).distance_km; } catch (...) { continue; }
        rows.push_back({km, &n});
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.km < b.km; });
    QVariantList out;
    for (size_t i = 0; i < rows.size() && int(i) < kShown; ++i)
        out.push_back(QVariantMap{{"name", rows[i].n->short_name.isEmpty() ? rows[i].n->id : rows[i].n->short_name},
                                  {"longName", rows[i].n->long_name}, {"km", int(std::lround(rows[i].km))}});
    return out;
}

void register_widget_mesh(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new MeshModel(context, make_tcp_socket_factory());
    registry.add({"mesh", "Meshtastic", QUrl("qrc:/widgets/mesh/MeshCard.qml"), "side", 165}, model);
    QTimer::singleShot(0, model, &MeshModel::start);
}

} // namespace hamclock
