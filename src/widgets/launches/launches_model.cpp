#include "launches_model.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// Launch Library 2 free tier allows 15 requests per hour per IP; one an hour.
constexpr PollPolicy kPolicy{60min, 6h, 6h};
constexpr char kUrl[] = "https://ll.thespacedevs.com/2.2.0/launch/upcoming/?limit=10&mode=list";
constexpr int kLaterRows = 3;

QString when_text(const launches::Launch& l, const QDateTime& now) {
    return launches::countdown_text(l.net, now) + (l.precise ? "" : " (approx)");
}
} // namespace

LaunchesModel::LaunchesModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("launches", "Launches", parent),
      source_(new PolledSource("launch-library-upcoming", QUrl(kUrl), kPolicy, context.transport, this)) {
    connect(source_, &PolledSource::updated, this, [this] {
        feed_ = launches::parse(source_->body());
        emit launchesChanged();
        refreshState();
    });
    connect(source_, &PolledSource::failed, this, &LaunchesModel::refreshState);
    connect(this, &WidgetModel::ageChanged, this, [this] { emit launchesChanged(); refreshState(); });
}

void LaunchesModel::start() { source_->start(); }

bool LaunchesModel::hasNext() const { return !launches::upcoming(feed_, now(), 1).empty(); }

QString LaunchesModel::nextName() const {
    const auto u = launches::upcoming(feed_, now(), 1);
    return u.empty() ? QString() : u.front().name;
}

QString LaunchesModel::nextCountdown() const {
    const auto u = launches::upcoming(feed_, now(), 1);
    return u.empty() ? QString() : when_text(u.front(), now());
}

QString LaunchesModel::nextDetail() const {
    const auto u = launches::upcoming(feed_, now(), 1);
    if (u.empty()) return {};
    QStringList parts;
    for (const auto& p : {u.front().provider, u.front().location, u.front().status})
        if (!p.isEmpty()) parts << p;
    return parts.join(" · ");
}

QVariantList LaunchesModel::later() const {
    QVariantList out;
    const auto u = launches::upcoming(feed_, now(), 1 + kLaterRows);
    for (size_t i = 1; i < u.size(); ++i)
        out.push_back(QVariantMap{{"name", u[i].name}, {"when", when_text(u[i], now())}, {"status", u[i].status}});
    return out;
}

void LaunchesModel::refreshState() {
    if (!feed_.ok) {
        if (!source_->lastError().isEmpty()) setState("offline", "Launch Library unreachable: " + source_->lastError());
        else if (source_->fetchedAt().isValid()) setState("stale", "Launch Library returned no usable launch data");
        else setState("loading", "Contacting Launch Library…");
        return;
    }
    const auto fetched = source_->fetchedAt();
    if (source_->isStale(now())) setState("stale", "Launch Library 2 (out of date)", fetched);
    else setState("live", "Launch Library 2 (The Space Devs)", fetched);
}

void register_widget_launches(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new LaunchesModel(context);
    registry.add({"launches", "Launches", QUrl("qrc:/widgets/launches/LaunchesCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &LaunchesModel::start);
}

} // namespace hamclock
