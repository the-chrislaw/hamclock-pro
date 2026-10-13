#include "dx_cluster_model.hpp"
#include "../../ui/app_settings.hpp"

namespace hamclock {

QVariant DxSpotList::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
    const auto& s = spots_[size_t(index.row())];
    switch (role) {
    case DxRole: return s.dx;
    case SpotterRole: return s.spotter;
    case FrequencyRole: return QString::number(s.frequency_khz, 'f', 1);
    case BandRole: return s.band;
    case ModeRole: return s.mode;
    case TimeRole: return s.time.toString("HH:mm") + "Z";
    case CommentRole: return s.comment;
    }
    return {};
}

QHash<int, QByteArray> DxSpotList::roleNames() const {
    return {{DxRole, "dx"}, {SpotterRole, "spotter"}, {FrequencyRole, "frequency"}, {BandRole, "band"},
            {ModeRole, "mode"}, {TimeRole, "time"}, {CommentRole, "comment"}};
}

void DxSpotList::add(const dxcluster::DxSpot& spot) {
    beginInsertRows({}, 0, 0);
    spots_.push_front(spot);
    endInsertRows();
    if (int(spots_.size()) > kCapacity) {
        beginRemoveRows({}, kCapacity, kCapacity);
        spots_.pop_back();
        endRemoveRows();
    }
}

DxClusterModel::DxClusterModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent)
    : WidgetModel("dxcluster", "DX cluster", parent), settings_(context.settings),
      client_(std::move(factory), this), list_(this) {
    connect(&client_, &TelnetClient::lineReceived, this, &DxClusterModel::on_line);
    connect(&client_, &TelnetClient::stateChanged, this, &DxClusterModel::on_state);
}

void DxClusterModel::start() {
    const QString host = settings_ ? settings_->clusterHost() : QString();
    const QString call = settings_ ? settings_->callsign() : QString();
    client_.start(host, settings_ ? settings_->clusterPort() : 0, call);
    if (client_.state() == TelnetClient::State::Idle)
        setState("unconfigured", host.isEmpty() ? "Not configured: set a cluster host in Settings"
                                                : "Not configured: set your callsign in Settings");
}

void DxClusterModel::on_state(TelnetClient::State state) {
    using S = TelnetClient::State;
    switch (state) {
    case S::Connecting: case S::LoggingIn: setState("loading", "Connecting to cluster…"); break;
    case S::Online: setState("live", "Cluster " + (settings_ ? settings_->clusterHost() : QString()), now()); break;
    case S::Waiting: setState("offline", "Cluster connection lost; retrying"); break;
    case S::Idle: break;
    }
}

void DxClusterModel::on_line(const QString& line) {
    const auto spot = dxcluster::parse_spot_line(line, now());
    if (!spot) return;
    list_.add(*spot);
    emit countChanged();
    setState("live", "Cluster " + (settings_ ? settings_->clusterHost() : QString()), now());
}

void register_widget_dxcluster(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new DxClusterModel(context, make_tcp_socket_factory());
    registry.add({"dxcluster", "DX cluster", QUrl("qrc:/widgets/dxcluster/DxClusterCard.qml"), "wide", 40}, model);
    QTimer::singleShot(0, model, &DxClusterModel::start);
}

} // namespace hamclock
