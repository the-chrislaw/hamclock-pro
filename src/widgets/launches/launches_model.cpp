#include "launches_model.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// Launch Library 2 free tier allows 15 requests per hour per IP; one an hour.
constexpr PollPolicy kPolicy{60min, 6h, 6h};
constexpr char kUrl[] = "https://ll.thespacedevs.com/2.2.0/launch/upcoming/?limit=10&mode=list";
constexpr int kLaterRows = 9;  // the feed asks for ten; a tall pane lists as many as fit, regular layouts three

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
        updateMapLayer();
    });
    connect(source_, &PolledSource::failed, this, &LaunchesModel::refreshState);
    connect(this, &WidgetModel::ageChanged, this, [this] { emit launchesChanged(); refreshState(); updateMapLayer(); });
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
    const auto hint = launches::ham_hint(u.front());
    if (!hint.isEmpty()) parts << hint;
    return parts.join(" · ");
}

QVariantList LaunchesModel::later() const {
    QVariantList out;
    const auto u = launches::upcoming(feed_, now(), 1 + kLaterRows);
    for (size_t i = 1; i < u.size(); ++i)
        out.push_back(QVariantMap{{"name", u[i].name}, {"when", when_text(u[i], now())}, {"status", u[i].status},
                                    {"ham", !launches::ham_hint(u[i]).isEmpty()}});
    return out;
}

// One marker per site with an upcoming launch (the soonest one labels it).
// Removed when there is no usable list, so stale pads never linger.
void LaunchesModel::updateMapLayer() {
    OverlayLayer layer;
    layer.id = "launches";
    layer.labels = OverlayLabelMode::Call;
    layer.min_label_distance_px = 40;
    if (feed_.ok && !source_->isStale(now())) {
        for (const auto& l : launches::upcoming(feed_, now(), 10)) {
            const auto pos = launches::pad_position(l);
            if (!pos) continue;
            bool dup = false;
            for (const auto& m : layer.markers)
                if (qAbs(m.where.latitude - pos->lat) < 0.05 && qAbs(m.where.longitude - pos->lon) < 0.05) dup = true;
            if (dup) continue;
            OverlayMarker m;
            m.where = {pos->lat, pos->lon};
            m.call = l.name.section(" | ", -1);
            m.detail = l.name + " · " + when_text(l, now());
            m.color = QColor("#ff9f1c");
            m.shape = OverlayShape::Dot;
            m.size_px = 4;
            layer.markers.push_back(m);
        }
    }
    if (layer.markers.empty()) MapOverlays::instance().remove_layer("launches");
    else MapOverlays::instance().set_layer(layer);
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
