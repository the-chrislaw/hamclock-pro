#include "aprs_model.hpp"
#include "../../domain/geometry.hpp"
#include "../../ui/app_settings.hpp"
#include <algorithm>

namespace hamclock {

AprsModel::AprsModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent)
    : WidgetModel("aprs", "Nearby APRS", parent), settings_(context.settings), factory_(std::move(factory)) {
    retry_.setSingleShot(true);
    connect(&retry_, &QTimer::timeout, this, &AprsModel::connect_now);
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, &AprsModel::start);
}

QString AprsModel::login_line(const QString& call, double lat, double lon, int radius_km) {
    return QString("user %1 pass -1 vers hamclock-pro 0.1 filter r/%2/%3/%4\r\n")
        .arg(call).arg(lat, 0, 'f', 2).arg(lon, 0, 'f', 2).arg(radius_km);
}

void AprsModel::start() {
    wanted_ = false;
    retry_.stop();
    if (socket_) { socket_->close(); socket_->deleteLater(); socket_ = nullptr; }
    stations_.clear();
    partial_.clear();
    emit changed();
    const QString call = settings_ ? settings_->callsign().trimmed().toUpper() : QString();
    if (call.isEmpty()) { setState("unconfigured", "Not configured: set your callsign in Settings"); return; }
    for (QChar c : call)
        if (!(c.isDigit() || (c.unicode() < 128 && c.isLetter()) || c == '-')) {
            setState("error", "Callsign cannot be used for APRS-IS login (portable suffixes unsupported)");
            return;
        }
    login_ = login_line(call, settings_->deLatitude(), settings_->deLongitude(), kRadiusKm);
    wanted_ = true;
    failures_ = 0;
    connect_now();
}

void AprsModel::connect_now() {
    if (!wanted_) return;
    if (socket_) { socket_->close(); socket_->deleteLater(); }
    socket_ = factory_(this);
    partial_.clear();
    setState("loading", "Connecting to APRS-IS…");
    connect(socket_, &TelnetSocket::connected, this, [this] { socket_->write(login_.toLatin1()); });
    connect(socket_, &TelnetSocket::dataReceived, this, &AprsModel::on_data);
    connect(socket_, &TelnetSocket::closed, this, [this](const QString&) { on_closed(); });
    socket_->connectToHost(kHost, kPort);
}

void AprsModel::on_closed() {
    if (!wanted_) return;
    ++failures_;
    setState("offline", "APRS-IS connection lost; retrying");
    retry_.start(int(TelnetClient::backoff(failures_).count()) * 1000);
}

void AprsModel::on_data(const QByteArray& data) {
    partial_.append(data);
    if (partial_.size() > 8192) partial_.clear(); // no newline in sight: not APRS-IS
    int nl;
    bool any = false;
    while ((nl = partial_.indexOf('\n')) >= 0) {
        const QString line = QString::fromLatin1(partial_.left(nl)).trimmed();
        partial_.remove(0, nl + 1);
        if (line.startsWith('#')) {
            if (line.contains("logresp") && line.contains("verified")) failures_ = 0;
            setState("live", "APRS-IS", now()); // server banner/keepalive: connection is up
            continue;
        }
        const auto st = aprs::parse_packet(line);
        if (!st) continue;
        stations_.insert(st->call, {*st, now()});
        any = true;
    }
    if (any) {
        prune();
        setState("live", "APRS-IS", now());
        emit changed();
    }
}

void AprsModel::prune() {
    const QDateTime cutoff = now().addSecs(-kMaxAgeSec);
    for (auto it = stations_.begin(); it != stations_.end();)
        it = it->heard < cutoff ? stations_.erase(it) : ++it;
    while (int(stations_.size()) > kCapacity) {
        auto oldest = stations_.begin();
        for (auto it = stations_.begin(); it != stations_.end(); ++it)
            if (it->heard < oldest->heard) oldest = it;
        stations_.erase(oldest);
    }
}

QVariantList AprsModel::nearest() const {
    struct Row { double km; const aprs::Station* s; };
    std::vector<Row> rows;
    const Location de{settings_ ? settings_->deLatitude() : 0, settings_ ? settings_->deLongitude() : 0};
    for (const auto& e : stations_) {
        double km = 0;
        try { km = geometry(de, {e.station.latitude, e.station.longitude}).distance_km; } catch (...) { continue; }
        rows.push_back({km, &e.station});
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.km < b.km; });
    QVariantList out;
    for (size_t i = 0; i < rows.size() && int(i) < kShown; ++i)
        out.push_back(QVariantMap{{"call", rows[i].s->call}, {"km", int(std::lround(rows[i].km))},
                                  {"symbol", rows[i].s->symbol}, {"comment", rows[i].s->comment}});
    return out;
}

void register_widget_aprs(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new AprsModel(context, make_tcp_socket_factory());
    registry.add({"aprs", "Nearby APRS", QUrl("qrc:/widgets/aprs/AprsCard.qml"), "side", 160}, model);
    QTimer::singleShot(0, model, &AprsModel::start);
}

} // namespace hamclock
