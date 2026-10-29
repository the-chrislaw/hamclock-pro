#include "hamsat_model.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// Public read endpoint; alerts are posted by people, so a 10 minute poll is plenty.
constexpr PollPolicy kPolicy{10min, 1h, 1h};
constexpr char kUrl[] = "https://hams.at/api/alerts/upcoming";
constexpr int kLaterRows = 10;  // a tall pane lists as many as fit; regular layouts show three

QString title_text(const hamsat::Activation& a) { return a.callsign + " on " + a.satellite; }
} // namespace

HamsatModel::HamsatModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("hamsat", "hams.at", parent), settings_(context.settings) {
    // With an API key (and a bearer-capable transport) the request is authenticated so hams.at
    // adapts the passes to the user's station; otherwise the public list is read.
    Transport transport = context.transport;
    if (context.auth_transport) {
        transport = [this, auth = context.auth_transport](const QUrl& url, FetchDone done) {
            const QByteArray key = settings_ && !key_rejected_ ? settings_->hamsatKey().toUtf8() : QByteArray();
            attempt_with_key_ = !key.isEmpty();
            auth(url, key, std::move(done));
        };
    }
    source_ = new PolledSource("hamsat-upcoming", QUrl(kUrl), kPolicy, transport, this);
    connect(source_, &PolledSource::updated, this, [this](bool from_cache) {
        feed_ = hamsat::parse(source_->body());
        // A cached list does not say how it was fetched, so it is never shown as personalised.
        fetched_with_key_ = !from_cache && attempt_with_key_;
        emit activationsChanged();
        refreshState();
    });
    connect(source_, &PolledSource::failed, this, [this](const QString& error) {
        // A rejected key is dropped until it changes, and the public list is fetched instead.
        if (attempt_with_key_ && error.startsWith("HTTP 401")) {
            key_rejected_ = true;
            source_->refreshNow();
        }
        refreshState();
    });
    if (settings_) connect(settings_, &AppSettings::accountsChanged, this, [this] {
        const bool has_key = !settings_->hamsatKey().isEmpty();
        if (has_key != attempt_with_key_ || key_rejected_) {
            key_rejected_ = false;
            source_->refreshNow();
        }
    });
    connect(this, &WidgetModel::ageChanged, this, [this] { emit activationsChanged(); refreshState(); });
}

void HamsatModel::start() { source_->start(); }

bool HamsatModel::hasNext() const { return !hamsat::upcoming(feed_, now(), 1).empty(); }

QString HamsatModel::nextWhen() const {
    const auto u = hamsat::upcoming(feed_, now(), 1);
    return u.empty() ? QString() : hamsat::when_text(u.front(), now());
}

QString HamsatModel::nextTitle() const {
    const auto u = hamsat::upcoming(feed_, now(), 1);
    return u.empty() ? QString() : title_text(u.front());
}

QString HamsatModel::nextDetail() const {
    const auto u = hamsat::upcoming(feed_, now(), 1);
    if (u.empty()) return {};
    const auto& a = u.front();
    QStringList parts;
    if (!a.mode.isEmpty()) parts << a.mode;
    if (a.mhz > 0) parts << QString::number(a.mhz, 'f', 3) + " MHz";
    if (!a.grids.isEmpty()) parts << a.grids;
    parts << a.aos.toString("HH:mm") + "Z-" + a.los.toString("HH:mm") + "Z";
    const auto here = station_text(a);
    if (!here.isEmpty()) parts << here;
    return parts.join(" · ");
}

QString HamsatModel::station_text(const hamsat::Activation& a) const {
    if (!personalised()) return {};
    QString el = a.max_elevation >= 0 ? QString(" max el %1°").arg(qRound(a.max_elevation)) : QString();
    if (a.workable == 1) {
        if (a.workable_start.isValid() && a.workable_end.isValid())
            return "workable " + a.workable_start.toString("HH:mm") + "Z-" + a.workable_end.toString("HH:mm") + "Z" + el;
        return "workable" + el;
    }
    if (a.workable == 0 && a.max_elevation >= 0) return "not workable here" + el;
    return {};
}

QVariantList HamsatModel::later() const {
    QVariantList out;
    const auto u = hamsat::upcoming(feed_, now(), 1 + kLaterRows);
    for (size_t i = 1; i < u.size(); ++i)
        out.push_back(QVariantMap{{"when", hamsat::when_text(u[i], now())}, {"title", title_text(u[i])}});
    return out;
}

void HamsatModel::refreshState() {
    if (!feed_.ok) {
        if (!source_->lastError().isEmpty()) setState("offline", "hams.at unreachable: " + source_->lastError());
        else if (source_->fetchedAt().isValid()) setState("stale", "hams.at returned no usable activation data");
        else setState("loading", "Contacting hams.at…");
        return;
    }
    const auto fetched = source_->fetchedAt();
    if (source_->isStale(now())) setState("stale", "hams.at (out of date)", fetched);
    else if (key_rejected_) setState("live", "hams.at public list (the API key was rejected; check Settings)", fetched);
    else setState("live", personalised() ? "hams.at activation alerts (your station)" : "hams.at activation alerts", fetched);
}

void register_widget_hamsat(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new HamsatModel(context);
    registry.add({"hamsat", "hams.at", QUrl("qrc:/widgets/hamsat/HamsatCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &HamsatModel::start);
}

} // namespace hamclock
