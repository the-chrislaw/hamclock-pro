#include "firewx_model.hpp"
#include "../../domain/geometry.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include <QTimer>
#include <QUrlQuery>
#include <algorithm>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// api.weather.gov asks for sensible polling; fire-weather products change a few times
// an hour at most, so 10 minutes is plenty.
constexpr PollPolicy kPolicy{10min, 1h, 3h};

QUrl feed_url() {
    QUrl url("https://api.weather.gov/alerts/active");
    QUrlQuery q;
    q.addQueryItem("message_type", "alert");
    q.addQueryItem("event", firewx::requested_events());
    url.setQuery(q);
    return url;
}
// Alerts covering the saved DE point. NWS accepts at most 4 decimals.
QUrl point_url(double lat, double lon) {
    QUrl url("https://api.weather.gov/alerts/active");
    QUrlQuery q;
    q.addQueryItem("point", QString::number(lat, 'f', 4) + "," + QString::number(lon, 'f', 4));
    q.addQueryItem("message_type", "alert");
    q.addQueryItem("event", firewx::requested_events());
    url.setQuery(q);
    return url;
}
QString short_event(QString s) {
    return s.replace("Extreme Fire Danger", "Extreme Fire").replace(" Warning", "").replace("Fire Weather Watch", "Fire Watch");
}
QString stamp(const QDateTime& t) { return t.toString("dd HH:mm") + "Z"; }
} // namespace

FirewxModel::FirewxModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("firewx", "Fire weather", parent),
      context_(context),
      source_(new PolledSource("nws-firewx-alerts", feed_url(), kPolicy, context.transport, this)) {
    // Unusable answers (schema change, cut-off body) keep the last good data.
    source_->setValidator([](const QByteArray& body) { return firewx::parse(body).ok; });
    connect(source_, &PolledSource::updated, this, [this] {
        feed_ = firewx::parse(source_->body());
        emit alertsChanged();
        refreshState();
    });
    connect(source_, &PolledSource::failed, this, &FirewxModel::refreshState);
    if (context_.settings) connect(context_.settings, &AppSettings::stationChanged, this, &FirewxModel::resetPointSource);
    connect(this, &WidgetModel::ageChanged, this, [this] { emit alertsChanged(); refreshState(); });
}

void FirewxModel::start() {
    source_->start();
    resetPointSource();
}

void FirewxModel::resetPointSource() {
    if (point_) { point_->deleteLater(); point_ = nullptr; }
    point_feed_ = {};
    const auto* s = context_.settings;
    if (s && s->configured() && valid_location({s->deLatitude(), s->deLongitude()})) {
        point_ = new PolledSource("nws-firewx-point", point_url(s->deLatitude(), s->deLongitude()), kPolicy,
                                  context_.transport, this);
        point_->setValidator([](const QByteArray& body) { return firewx::parse(body).ok; });
        connect(point_, &PolledSource::updated, this, [this] {
            point_feed_ = firewx::parse(point_->body());
            emit alertsChanged();
        });
        point_->start();
    }
    emit alertsChanged();
}

QString FirewxModel::atDe() const {
    if (!point_ || !point_feed_.ok) return {};
    // A point body older than the shared staleness limit must not read as "none active".
    if (point_->isStale(now())) return {};
    const firewx::Alert* top = nullptr;
    for (const auto& a : point_feed_.alerts)
        if (a.expires > now() && (!top || a.rank > top->rank)) top = &a;
    if (!top) return "At DE: none active";
    return "At DE: " + short_event(top->event) + " · until " + stamp(top->expires);
}

std::vector<firewx::Alert> FirewxModel::active() const {
    std::vector<firewx::Alert> out;
    for (const auto& a : feed_.alerts) if (a.expires > now()) out.push_back(a);
    return out;
}

int FirewxModel::topRank() const { const auto a = active(); return a.empty() ? 0 : a.front().rank; }
QString FirewxModel::headline() const { const auto a = active(); return a.empty() ? QString() : a.front().event; }
QString FirewxModel::shortHeadline() const { return short_event(headline()); }
QString FirewxModel::headlineArea() const { const auto a = active(); return a.empty() ? QString() : a.front().area; }

QString FirewxModel::headlineTimes() const {
    const auto a = active();
    if (a.empty()) return {};
    QStringList parts;
    if (a.front().sent.isValid()) parts << "issued " + stamp(a.front().sent);
    parts << "until " + stamp(a.front().expires);
    return parts.join(" · ");
}

QString FirewxModel::headlineStart() const {
    const auto a = active();
    return !a.empty() && a.front().onset.isValid() && a.front().onset > now() ? "starts " + stamp(a.front().onset) : QString();
}

QString FirewxModel::summary() const {
    // active() is sorted worst first, so first appearance order is severity order.
    std::vector<std::pair<QString, int>> groups;
    for (const auto& a : active()) {
        auto it = std::find_if(groups.begin(), groups.end(), [&](const auto& g) { return g.first == a.event; });
        if (it == groups.end()) groups.emplace_back(a.event, 1); else ++it->second;
    }
    QStringList parts;
    for (const auto& g : groups) parts << QString::number(g.second) + " " + g.first;
    return parts.join(" · ");
}

void FirewxModel::refreshState() {
    if (!feed_.ok) {
        if (!source_->lastError().isEmpty()) setState("offline", "NWS unreachable: " + source_->lastError());
        else if (source_->fetchedAt().isValid()) setState("stale", "NWS returned no usable alert data");
        else setState("loading", "Contacting NWS…");
        return;
    }
    const auto fetched = source_->fetchedAt();
    if (source_->isStale(now())) setState("stale", "NWS fire-weather alerts (list out of date)", fetched);
    else setState("live", "NWS fire-weather warnings and watches (US)", fetched);
}

void register_widget_firewx(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new FirewxModel(context);
    registry.add({"firewx", "Fire weather", QUrl("qrc:/widgets/firewx/FirewxCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &FirewxModel::start);
}

} // namespace hamclock
