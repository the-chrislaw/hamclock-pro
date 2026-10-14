#include "marine_model.hpp"
#include "../../net/polled_source.hpp"
#include "../../domain/geometry.hpp"
#include "../../ui/app_settings.hpp"
#include <QTimer>
#include <QUrlQuery>
#include <algorithm>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// api.weather.gov asks for sensible polling; marine products change a few times
// an hour at most, so 10 minutes is plenty.
constexpr PollPolicy kPolicy{10min, 1h, 3h};

QUrl feed_url() {
    QUrl url("https://api.weather.gov/alerts/active");
    QUrlQuery q;
    q.addQueryItem("message_type", "alert");
    q.addQueryItem("event", marine::requested_events());
    url.setQuery(q);
    return url;
}
QUrl region_url(const QString& code) {
    QUrl url("https://api.weather.gov/alerts/active");
    QUrlQuery q;
    q.addQueryItem("region", code);
    q.addQueryItem("message_type", "alert");
    q.addQueryItem("event", marine::requested_events());
    url.setQuery(q);
    return url;
}
QString stamp(const QDateTime& t) { return t.toString("dd HH:mm") + "Z"; }
} // namespace

MarineModel::MarineModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("marine", "Marine warnings", parent),
      context_(context),
      source_(new PolledSource("nws-marine-alerts", feed_url(), kPolicy, context.transport, this)) {
    connect(source_, &PolledSource::updated, this, [this] {
        feed_ = marine::parse(source_->body());
        emit alertsChanged();
        refreshState();
    });
    connect(source_, &PolledSource::failed, this, &MarineModel::refreshState);
    if (context_.settings) connect(context_.settings, &AppSettings::stationChanged, this, &MarineModel::resetRegionSource);
    connect(this, &WidgetModel::ageChanged, this, [this] { emit alertsChanged(); refreshState(); });
}

void MarineModel::start() {
    source_->start();
    resetRegionSource();
}

void MarineModel::resetRegionSource() {
    if (region_) { region_->deleteLater(); region_ = nullptr; }
    region_feed_ = {};
    region_code_.clear();
    const auto* s = context_.settings;
    if (s && s->configured() && valid_location({s->deLatitude(), s->deLongitude()})) {
        region_code_ = marine::region_for(s->deLatitude(), s->deLongitude());
        if (!region_code_.isEmpty()) {
            region_ = new PolledSource("nws-marine-region", region_url(region_code_), kPolicy, context_.transport, this);
            connect(region_, &PolledSource::updated, this, [this] {
                region_feed_ = marine::parse(region_->body());
                emit alertsChanged();
            });
            region_->start();
        }
    }
    emit alertsChanged();
}

QString MarineModel::nearDe() const {
    // Data older than the shared staleness limit must not read as "none active".
    if (!region_ || !region_feed_.ok || region_->isStale(now())) return {};
    std::vector<marine::Alert> act;
    for (const auto& a : region_feed_.alerts) if (a.expires > now()) act.push_back(a);
    const QString name = marine::region_name(region_code_);
    if (act.empty()) return name + ": none active";
    return name + ": " + QString::number(act.size()) + " · " + act.front().event + " until " + stamp(act.front().expires);
}

std::vector<marine::Alert> MarineModel::active() const {
    std::vector<marine::Alert> out;
    for (const auto& a : feed_.alerts) if (a.expires > now()) out.push_back(a);
    return out;
}

int MarineModel::topRank() const { const auto a = active(); return a.empty() ? 0 : a.front().rank; }
QString MarineModel::headline() const { const auto a = active(); return a.empty() ? QString() : a.front().event; }
QString MarineModel::headlineArea() const { const auto a = active(); return a.empty() ? QString() : a.front().area; }

QString MarineModel::headlineTimes() const {
    const auto a = active();
    if (a.empty()) return {};
    QStringList parts;
    if (a.front().sent.isValid()) parts << "issued " + stamp(a.front().sent);
    parts << "until " + stamp(a.front().expires);
    return parts.join(" · ");
}

QString MarineModel::summary() const {
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

void MarineModel::refreshState() {
    if (!feed_.ok) {
        if (!source_->lastError().isEmpty()) setState("offline", "NWS unreachable: " + source_->lastError());
        else if (source_->fetchedAt().isValid()) setState("stale", "NWS returned no usable alert data");
        else setState("loading", "Contacting NWS…");
        return;
    }
    const auto fetched = source_->fetchedAt();
    if (source_->isStale(now())) setState("stale", "NWS marine alerts (list out of date)", fetched);
    else setState("live", "NWS marine warnings and advisories (US waters)", fetched);
}

void register_widget_marine(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new MarineModel(context);
    registry.add({"marine", "Marine warnings", QUrl("qrc:/widgets/marine/MarineCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &MarineModel::start);
}

} // namespace hamclock
