#include "solar_aurora_model.hpp"
#include "../../map/styles.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
constexpr PollPolicy kPolicy{10min, 1h, 30min};
constexpr char kUrl[] = "https://services.swpc.noaa.gov/json/ovation_aurora_latest.json";
constexpr int kMaxAgeSecs = 3 * 3600;
} // namespace

SolarAuroraModel::SolarAuroraModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solaraurora", "Aurora", parent),
      source_(new PolledSource("swpc-ovation-aurora", QUrl(kUrl), kPolicy, context.transport, this)) {
    trackSource(*source_, "NOAA SWPC OVATION");
    connect(source_, &PolledSource::updated, this, &SolarAuroraModel::parse);
}

SolarAuroraModel::~SolarAuroraModel() { MapStyles::instance().remove_style("aurora"); }

void SolarAuroraModel::start() { source_->start(); }

QString SolarAuroraModel::observed() const { return valid_ ? grid_.observed.toString("HH:mm") + "Z" : QString(); }

void SolarAuroraModel::parse() {
    grid_ = solaraurora::parse_ovation(source_->body());
    valid_ = grid_.valid && grid_.observed.secsTo(now()) <= kMaxAgeSecs;
    north_ = valid_ ? solaraurora::peak(grid_, true) : 0;
    south_ = valid_ ? solaraurora::peak(grid_, false) : 0;
    if (valid_) {
        MapStyle style;
        style.id = "aurora";
        style.name = "Aurora";
        style.image = solaraurora::to_image(grid_);
        style.opacity = 0.75;
        style.legend = solaraurora::aurora_scale();
        style.credit = "NOAA SWPC OVATION";
        MapStyles::instance().set_style(style);
    } else {
        MapStyles::instance().remove_style("aurora");  // never leave an old nowcast on the map
    }
    emit valuesChanged();
}

void register_widget_solaraurora(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SolarAuroraModel(context);
    registry.add({"solaraurora", "Aurora", QUrl("qrc:/widgets/solaraurora/SolarAuroraCard.qml"), "strip", 57}, model);
    QTimer::singleShot(0, model, &SolarAuroraModel::start);
}

} // namespace hamclock
