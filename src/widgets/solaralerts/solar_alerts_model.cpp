#include "solar_alerts_model.hpp"
#include "../../net/polled_source.hpp"
#include <QJsonDocument>
#include <QTimer>
#include <QVariantMap>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
constexpr PollPolicy kPolicy{10min, 2h, 1h};
constexpr char kUrl[] = "https://services.swpc.noaa.gov/products/alerts.json";
} // namespace

SolarAlertsModel::SolarAlertsModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solaralerts", "Space alerts", parent),
      source_(new PolledSource("swpc-alerts", QUrl(kUrl), kPolicy, context.transport, this)) {
    trackSource(*source_, "NOAA SWPC");
    connect(source_, &PolledSource::updated, this, &SolarAlertsModel::parse);
    // Expiry is time-driven: re-evaluate what is in force as the clock moves.
    connect(this, &WidgetModel::ageChanged, this, [this] {
        if (!has_data_) return;
        active_ = solaralerts::active_alerts(all_, now());
        emit valuesChanged();
    });
}

void SolarAlertsModel::start() { source_->start(); }

void SolarAlertsModel::parse() {
    all_ = solaralerts::parse_alerts(source_->body());
    has_data_ = !source_->body().isEmpty() && QJsonDocument::fromJson(source_->body()).isArray();
    active_ = solaralerts::active_alerts(all_, now());
    emit valuesChanged();
}

QVariantList SolarAlertsModel::alerts() const {
    QVariantList out;
    for (const auto& a : active_)
        out.push_back(QVariantMap{
            {"kind", a.kind}, {"headline", a.headline}, {"scale", a.scale},
            {"level", solaralerts::alert_level(a)},
            {"issued", a.issued.toString("HH:mm") + "Z"},
            {"until", a.until.isValid() ? "until " + a.until.toString("HH:mm") + "Z" : QString()}});
    return out;
}

void register_widget_solaralerts(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SolarAlertsModel(context);
    registry.add({"solaralerts", "Space alerts", QUrl("qrc:/widgets/solaralerts/SolarAlertsCard.qml"), "strip", 55},
                 model);
    QTimer::singleShot(0, model, &SolarAlertsModel::start);
}

} // namespace hamclock
