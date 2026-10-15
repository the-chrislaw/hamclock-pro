#include "hamsat_model.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// Public read endpoint; alerts are posted by people, so a 10 minute poll is plenty.
constexpr PollPolicy kPolicy{10min, 1h, 1h};
constexpr char kUrl[] = "https://hams.at/api/alerts/upcoming";
constexpr int kLaterRows = 3;

QString title_text(const hamsat::Activation& a) { return a.callsign + " on " + a.satellite; }
} // namespace

HamsatModel::HamsatModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("hamsat", "hams.at", parent),
      source_(new PolledSource("hamsat-upcoming", QUrl(kUrl), kPolicy, context.transport, this)) {
    connect(source_, &PolledSource::updated, this, [this] {
        feed_ = hamsat::parse(source_->body());
        emit activationsChanged();
        refreshState();
    });
    connect(source_, &PolledSource::failed, this, &HamsatModel::refreshState);
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
    return parts.join(" · ");
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
    else setState("live", "hams.at activation alerts", fetched);
}

void register_widget_hamsat(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new HamsatModel(context);
    registry.add({"hamsat", "hams.at", QUrl("qrc:/widgets/hamsat/HamsatCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &HamsatModel::start);
}

} // namespace hamclock
