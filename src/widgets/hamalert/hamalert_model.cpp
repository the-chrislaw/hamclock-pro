#include "hamalert_model.hpp"
#include "../dxcluster/spot_layer_style.hpp"
#include "../../ui/app_settings.hpp"
#include "../../map/overlay.hpp"
#include <QCryptographicHash>

namespace hamclock {
namespace {
const QString kRejectedText = QString::fromUtf8("HamAlert rejected the user name or password — check them in Settings");
} // namespace

HamAlertModel::HamAlertModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent)
    : WidgetModel("hamalert", "HamAlert", parent), settings_(context.settings), client_(std::move(factory), this) {
    if (settings_) {
        map_visible_ = settings_->mapSourceVisible("hamalert");
        connect(settings_, &AppSettings::optionsChanged, this, [this] {
            const bool v = settings_->mapSourceVisible("hamalert");
            if (v != map_visible_) setMapVisible(v);
            else publishOverlay();
        });
    }
    connect(&client_, &TelnetClient::lineReceived, this, &HamAlertModel::on_line);
    connect(&client_, &TelnetClient::stateChanged, this, &HamAlertModel::on_state);
    connect(&client_, &TelnetClient::loginRejected, this, &HamAlertModel::on_rejected);
    if (settings_) connect(settings_, &AppSettings::accountsChanged, this, &HamAlertModel::start);
}

HamAlertModel::~HamAlertModel() { MapOverlays::instance().remove_layer(kOverlayLayerId); }

void HamAlertModel::setMapVisible(bool visible) {
    map_visible_ = visible;
    publishOverlay();
}

// Alerted stations at their DXCC entity centre, only while the feed is live.
void HamAlertModel::publishOverlay() {
    if (!map_visible_ || state() != "live") { MapOverlays::instance().remove_layer(kOverlayLayerId); return; }
    auto layer = dxcluster::overlay_layer(spots_, resolver_);
    layer.id = kOverlayLayerId;
    for (auto& m : layer.markers) m.color = QColor("#e9c46a");
    apply_spot_label_style(layer, settings_, "hamalert");
    MapOverlays::instance().set_layer(layer);
}

void HamAlertModel::start() {
    const QString user = settings_ ? settings_->hamAlertUser() : QString();
    const QString password = settings_ ? settings_->hamAlertPassword() : QString();
    client_.stop();
    reached_ = false;
    spots_.clear();
    emit changed();
    publishOverlay();
    if (user.isEmpty() || password.isEmpty()) {
        setState("unconfigured", "Not configured: add your HamAlert login in Settings");
        return;
    }
    // A refused login is not tried again (the server would only refuse it again) until the user or password changes.
    tried_ = credentialsDigest(user, password);
    if (tried_ == rejected_) {
        setState("error", kRejectedText);
        return;
    }
    client_.start(kHost, kPort, user, {}, password);
    if (client_.state() == TelnetClient::State::Idle)
        setState("error", "HamAlert login contains characters the telnet feed cannot take");
}

void HamAlertModel::on_state(TelnetClient::State state) {
    using S = TelnetClient::State;
    switch (state) {
    case S::Connecting: case S::LoggingIn: setState("loading", "Connecting to HamAlert…"); break;
    case S::Online: reached_ = true; setState("live", "HamAlert", now()); break;
    case S::Waiting:
        setState("offline", reached_ ? "HamAlert connection lost; retrying"
                                     : QString("Cannot reach %1:%2; retrying").arg(kHost).arg(kPort));
        break;
    case S::Idle: break;
    }
    publishOverlay();
}

// A digest only, so the password is not kept a second time in memory.
QByteArray HamAlertModel::credentialsDigest(const QString& user, const QString& password) {
    return QCryptographicHash::hash((user + '\n' + password).toUtf8(), QCryptographicHash::Sha256);
}

void HamAlertModel::on_rejected() {
    rejected_ = tried_;
    setState("error", kRejectedText);
    publishOverlay();
}

void HamAlertModel::on_line(const QString& line) {
    const auto spot = dxcluster::parse_spot_line(line, now());
    if (!spot) return;
    spots_.push_front(*spot);
    if (int(spots_.size()) > kCapacity) spots_.pop_back();
    emit changed();
    setState("live", "HamAlert", now());
    publishOverlay();
}

QVariantList HamAlertModel::recent() const {
    QVariantList out;
    for (size_t i = 0; i < spots_.size() && int(i) < kShown; ++i) {
        const auto& s = spots_[i];
        out.push_back(QVariantMap{{"dx", s.dx}, {"frequency", QString::number(s.frequency_khz, 'f', 1)}, {"band", s.band},
                                  {"mode", s.mode}, {"time", s.time.toString("HH:mm") + "Z"}, {"comment", s.comment},
                                  {"ota", s.ota_ref.isEmpty() ? QString() : s.ota_program + " " + s.ota_ref}});
    }
    return out;
}

void register_widget_hamalert(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new HamAlertModel(context, make_tcp_socket_factory());
    registry.add({"hamalert", "HamAlert", QUrl("qrc:/widgets/hamalert/HamAlertCard.qml"), "side", 150}, model);
    QTimer::singleShot(0, model, &HamAlertModel::start);
}

} // namespace hamclock
