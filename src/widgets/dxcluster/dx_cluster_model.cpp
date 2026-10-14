#include "dx_cluster_model.hpp"
#include "../../ui/app_settings.hpp"
#include "../watchlist/watch_match.hpp"
#include "dxcc_table.hpp"
#include <algorithm>

namespace hamclock {

QVariant DxSpotList::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
    const auto& s = spots_[size_t(shown_[size_t(index.row())])];
    switch (role) {
    case DxRole: return s.dx;
    case SpotterRole: return s.spotter;
    case FrequencyRole: return QString::number(s.frequency_khz, 'f', 1);
    case BandRole: return s.band;
    case ModeRole: return s.mode;
    case TimeRole: return s.time.toString("HH:mm") + "Z";
    case CommentRole: return s.comment;
    case WatchedRole: return is_watched_[size_t(shown_[size_t(index.row())])] != 0;
    case CountryRole: {
        const auto* e = dxcluster::DxccTable::bundled().lookup(s.dx);
        return e ? e->name : QString();
    }
    }
    return {};
}

QHash<int, QByteArray> DxSpotList::roleNames() const {
    return {{DxRole, "dx"}, {SpotterRole, "spotter"}, {FrequencyRole, "frequency"}, {BandRole, "band"},
            {ModeRole, "mode"}, {TimeRole, "time"}, {CommentRole, "comment"}, {WatchedRole, "watched"}, {CountryRole, "country"}};
}

void DxSpotList::rebuild() {
    shown_.clear();
    for (size_t i = 0; i < spots_.size(); ++i)
        if (matches(spots_[i])) shown_.push_back(int(i));
}

void DxSpotList::set_filter(const QString& band, const QString& mode) {
    beginResetModel();
    band_ = band;
    mode_ = mode;
    rebuild();
    endResetModel();
}

void DxSpotList::add(const dxcluster::DxSpot& spot) {
    beginResetModel(); // at most kCapacity rows, so a reset is cheap and keeps indexes simple
    spots_.push_front(spot);
    is_watched_.insert(is_watched_.begin(), watchlist::matches(watch_, spot.dx));
    if (int(spots_.size()) > kCapacity) {
        spots_.pop_back();
        is_watched_.pop_back();
    }
    watched_ = int(std::count(is_watched_.begin(), is_watched_.end(), char(1)));
    rebuild();
    endResetModel();
}

void DxSpotList::set_watch(const QStringList& entries) {
    beginResetModel();
    watch_ = entries;
    is_watched_.assign(spots_.size(), 0);
    for (size_t i = 0; i < spots_.size(); ++i) is_watched_[i] = watchlist::matches(watch_, spots_[i].dx);
    watched_ = int(std::count(is_watched_.begin(), is_watched_.end(), char(1)));
    endResetModel();
}

void DxClusterModel::refreshWatchList() {
    list_.set_watch(settings_ ? settings_->watchList() : QStringList());
    emit countChanged();
}

QStringList DxClusterModel::bandChoices() {
    return {"", "160m", "80m", "60m", "40m", "30m", "20m", "17m", "15m", "12m", "10m", "6m", "2m"};
}

QStringList DxClusterModel::modeChoices() { return {"", "CW", "SSB", "FT8", "FT4", "RTTY"}; }

void DxClusterModel::setBandFilter(const QString& band) {
    if (!bandChoices().contains(band) || band == band_filter_) return;
    band_filter_ = band;
    list_.set_filter(band_filter_, mode_filter_);
    emit filterChanged();
    emit countChanged();
}

void DxClusterModel::setModeFilter(const QString& mode) {
    if (!modeChoices().contains(mode) || mode == mode_filter_) return;
    mode_filter_ = mode;
    list_.set_filter(band_filter_, mode_filter_);
    emit filterChanged();
    emit countChanged();
}

void DxClusterModel::cycleBandFilter() {
    const auto c = bandChoices();
    setBandFilter(c[(c.indexOf(band_filter_) + 1) % c.size()]);
}

void DxClusterModel::cycleModeFilter() {
    const auto c = modeChoices();
    setModeFilter(c[(c.indexOf(mode_filter_) + 1) % c.size()]);
}

DxClusterModel::DxClusterModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent)
    : WidgetModel("dxcluster", "DX cluster", parent), settings_(context.settings),
      client_(std::move(factory), this), list_(this), udp_(this) {
    connect(&client_, &TelnetClient::lineReceived, this, &DxClusterModel::on_line);
    connect(&client_, &TelnetClient::stateChanged, this, &DxClusterModel::on_state);
    if (settings_) {
        list_.set_watch(settings_->watchList());
        connect(settings_, &AppSettings::optionsChanged, this, &DxClusterModel::refreshWatchList);
        connect(settings_, &AppSettings::optionsChanged, this, &DxClusterModel::refreshUdp);
    }
    connect(&udp_, &dxcluster::UdpSpotListener::spotReceived, this, [this](const dxcluster::DxSpot& spot) {
        list_.add(spot);
        emit countChanged();
    });
}

void DxClusterModel::refreshUdp() {
    const int port = settings_ ? settings_->udpSpotPort() : 0;
    const bool lan = settings_ && settings_->udpSpotLan();
    if (port == udp_.port() && udp_.listening() == (port != 0)) return;
    if (!udp_.listen(port, lan)) qWarning("UDP spot input: could not bind port %d", port);
}

QString DxClusterModel::cluster_login(const QString& callsign, int ssid) {
    const QString call = callsign.trimmed().toUpper();
    if (call.isEmpty() || ssid < 1 || ssid > 99) return call;
    return call + '-' + QString::number(ssid);
}

void DxClusterModel::start() {
    refreshUdp();
    const QString host = settings_ ? settings_->clusterHost() : QString();
    const QString call = cluster_login(settings_ ? settings_->callsign() : QString(), settings_ ? settings_->clusterSsid() : 0);
    client_.start(host, settings_ ? settings_->clusterPort() : 0, call,
                  settings_ ? settings_->clusterCommands() : QStringList());
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
