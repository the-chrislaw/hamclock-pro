#include "hamalert_model.hpp"
#include "../../ui/app_settings.hpp"

namespace hamclock {

HamAlertModel::HamAlertModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent)
    : WidgetModel("hamalert", "HamAlert", parent), settings_(context.settings), client_(std::move(factory), this) {
    connect(&client_, &TelnetClient::lineReceived, this, &HamAlertModel::on_line);
    connect(&client_, &TelnetClient::stateChanged, this, &HamAlertModel::on_state);
    if (settings_) connect(settings_, &AppSettings::accountsChanged, this, &HamAlertModel::start);
}

void HamAlertModel::start() {
    const QString user = settings_ ? settings_->hamAlertUser() : QString();
    const QString password = settings_ ? settings_->hamAlertPassword() : QString();
    client_.stop();
    spots_.clear();
    emit changed();
    if (user.isEmpty() || password.isEmpty()) {
        setState("unconfigured", "Not configured: add your HamAlert login in Settings");
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
    case S::Online: setState("live", "HamAlert", now()); break;
    case S::Waiting: setState("offline", "HamAlert connection lost; retrying"); break;
    case S::Idle: break;
    }
}

void HamAlertModel::on_line(const QString& line) {
    const auto spot = dxcluster::parse_spot_line(line, now());
    if (!spot) return;
    spots_.push_front(*spot);
    if (int(spots_.size()) > kCapacity) spots_.pop_back();
    emit changed();
    setState("live", "HamAlert", now());
}

QVariantList HamAlertModel::recent() const {
    QVariantList out;
    for (size_t i = 0; i < spots_.size() && int(i) < kShown; ++i) {
        const auto& s = spots_[i];
        out.push_back(QVariantMap{{"dx", s.dx}, {"frequency", QString::number(s.frequency_khz, 'f', 1)}, {"band", s.band},
                                  {"mode", s.mode}, {"time", s.time.toString("HH:mm") + "Z"}, {"comment", s.comment}});
    }
    return out;
}

void register_widget_hamalert(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new HamAlertModel(context, make_tcp_socket_factory());
    registry.add({"hamalert", "HamAlert", QUrl("qrc:/widgets/hamalert/HamAlertCard.qml"), "side", 150}, model);
    QTimer::singleShot(0, model, &HamAlertModel::start);
}

} // namespace hamclock
