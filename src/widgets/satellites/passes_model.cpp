#include "passes_model.hpp"
#include "satellites_model.hpp"
#include "../../ui/app_settings.hpp"
#include <QDateTime>
#include <QSettings>
#include <QTimeZone>

namespace hamclock {

namespace {
constexpr double kHorizonS = 48 * 3600;
constexpr double kMinEl = 10.0;
constexpr int kMaxPasses = 12;
}

QString pass_day_label(qint64 unix_time, qint64 now) {
    const QDate day = QDateTime::fromSecsSinceEpoch(unix_time, QTimeZone::utc()).date();
    const QDate today = QDateTime::fromSecsSinceEpoch(now, QTimeZone::utc()).date();
    if (day == today) return "Today";
    if (day == today.addDays(1)) return "Tomorrow";
    return QLocale::c().toString(day, "ddd");
}

PassesModel::PassesModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("passes", "Passes", parent), settings_(context.settings) {
    timer_.setInterval(60 * 1000);
    connect(&timer_, &QTimer::timeout, this, &PassesModel::recompute);
    if (settings_) {
        connect(settings_, &AppSettings::stationChanged, this, &PassesModel::recompute);
        QSettings ini(settings_->optionsPath(), QSettings::IniFormat);
        const QString saved = ini.value("passes/filter").toString();
        if (saved == "amateur" || saved == "visible" || saved == "starred") filter_ = saved;
        starred_ = ini.value("passes/starred").toStringList();
        starred_.removeAll(QString());
    }
}

void PassesModel::start() { timer_.start(); recompute(); }

void PassesModel::setElements(const std::vector<Tle>& tles, const std::set<int>& amateur_norads) {
    tles_ = tles;
    amateur_ = amateur_norads;
    recompute();
}

void PassesModel::setFilter(const QString& filter) {
    const QString f = (filter == "amateur" || filter == "visible" || filter == "starred") ? filter : QString("all");
    if (f == filter_) return;
    filter_ = f;
    if (settings_) QSettings(settings_->optionsPath(), QSettings::IniFormat).setValue("passes/filter", f);
    recompute();
}

void PassesModel::toggleStar(const QString& name) {
    if (name.isEmpty()) return;
    if (!starred_.removeOne(name)) starred_.append(name);
    if (settings_) QSettings(settings_->optionsPath(), QSettings::IniFormat).setValue("passes/starred", starred_);
    recompute();
}

void PassesModel::recompute() {
    rows_.clear();
    empty_reason_.clear();
    const double t = static_cast<double>(now().toSecsSinceEpoch());
    if (!settings_ || !settings_->configured()) {
        empty_reason_ = "Set your station in Settings";
        setState("unconfigured", empty_reason_);
        emit changed();
        return;
    }
    const std::vector<Tle> tles = fresh_tles(tles_, t);
    if (tles.empty()) {
        empty_reason_ = tles_.empty() ? "Waiting for satellite elements" : "Satellite elements are too old";
        setState(tles_.empty() ? "loading" : "stale", empty_reason_);
        emit changed();
        return;
    }
    const Observer obs{settings_->deLatitude(), settings_->deLongitude(), 0.0};
    std::vector<SatellitePropagator> props;
    props.reserve(tles.size());
    std::vector<std::pair<std::string, const SatellitePropagator*>> sats;
    std::set<std::string> amateur_names;
    for (const Tle& tle : tles) props.emplace_back(tle);
    for (std::size_t i = 0; i < tles.size(); ++i) {
        if (!props[i].valid()) continue;
        const std::string name = satellite_short_name(QString::fromStdString(tles[i].name)).toStdString();
        sats.push_back({name, &props[i]});
        if (amateur_.count(tle_norad_id(tles[i]))) amateur_names.insert(name);
    }
    std::function<bool(const std::string&)> include;
    if (filter_ == "amateur") include = [&](const std::string& n) { return amateur_names.count(n) > 0; };
    if (filter_ == "starred") include = [&](const std::string& n) { return starred_.contains(QString::fromStdString(n)); };
    // Start a few minutes back so a pass in progress is still listed.
    const auto sched = pass_schedule(sats, obs, t - 900, kHorizonS + 900, kMinEl,
                                     filter_ == "visible" ? PassFilter::VisibleToEye : PassFilter::All,
                                     kMaxPasses * 2, include);
    const qint64 now_s = static_cast<qint64>(t);
    for (const ScheduledPass& s : sched) {
        if (s.pass.pass.los <= now_s) continue;
        const qint64 aos = s.pass.pass.aos;
        const QDateTime utc = QDateTime::fromSecsSinceEpoch(aos, QTimeZone::utc());
        rows_.append(QVariantMap{
            {"name", QString::fromStdString(s.name)}, {"aos", aos}, {"day", pass_day_label(aos, now_s)},
            {"utc", utc.toString("HH:mm") + "Z"},
            {"local", QDateTime::fromSecsSinceEpoch(aos).toString("HH:mm")},
            {"maxEl", qRound(s.pass.pass.max_el_deg)},
            {"from", QString::fromStdString(s.pass.aos_compass)}, {"to", QString::fromStdString(s.pass.los_compass)},
            {"duration", QString("%1 min").arg((s.pass.duration_s + 30) / 60)},
            {"mag", s.pass.brightest_mag ? QString::number(*s.pass.brightest_mag, 'f', 1) : QString()},
            {"visible", s.pass.visible_to_eye}, {"starred", starred_.contains(QString::fromStdString(s.name))}, {"live", aos <= now_s}});
        if (rows_.size() >= kMaxPasses) break;
    }
    if (rows_.isEmpty())
        empty_reason_ = filter_ == "visible" ? "No passes visible to the eye in 48 h"
                      : filter_ == "amateur" ? "No amateur-satellite passes in 48 h"
                      : filter_ == "starred" ? (starred_.isEmpty() ? "Tap a satellite name to star it" : "No starred-satellite passes in 48 h") : "No passes in 48 h";
    setState("live", "Computed offline from CelesTrak elements", now());
    emit changed();
}

void register_widget_passes(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new PassesModel(context);
    registry.add({"passes", "Passes", QUrl("qrc:/widgets/passes/PassesCard.qml"), "strip", 71}, model);
    // Fed by the Satellites pane (registered first) so the elements are downloaded once.
    if (auto* sats = qobject_cast<SatellitesModel*>(registry.model("satellites"))) {
        auto feed = [model, sats] { model->setElements(sats->elements(), sats->transmitterNorads()); };
        QObject::connect(sats, &SatellitesModel::elementsChanged, model, feed);
    }
    QTimer::singleShot(0, model, &PassesModel::start);
}

} // namespace hamclock
