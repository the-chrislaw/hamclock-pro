#include "solar_drap_model.hpp"
#include "../../map/styles.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
constexpr PollPolicy kPolicy{10min, 1h, 30min};
constexpr char kUrl[] = "https://services.swpc.noaa.gov/text/drap_global_frequencies.txt";
constexpr int kMaxAgeSecs = 2 * 3600;
} // namespace

SolarDrapModel::SolarDrapModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solardrap", "D-RAP", parent),
      source_(new PolledSource("swpc-drap", QUrl(kUrl), kPolicy, context.transport, this)) {
    trackSource(*source_, "NOAA SWPC D-RAP");
    connect(source_, &PolledSource::updated, this, &SolarDrapModel::parse);
}

SolarDrapModel::~SolarDrapModel() { MapStyles::instance().remove_style("drap"); }

void SolarDrapModel::start() { source_->start(); }

QString SolarDrapModel::observed() const { return valid_ ? grid_.valid_at.toString("HH:mm") + "Z" : QString(); }

QString SolarDrapModel::message() const {
    if (!valid_) return QString();
    return grid_.xray.isEmpty() ? grid_.proton : grid_.proton.isEmpty() ? grid_.xray : grid_.xray + " · " + grid_.proton;
}

void SolarDrapModel::parse() {
    grid_ = solardrap::parse_drap(source_->body());
    valid_ = grid_.valid && grid_.valid_at.secsTo(now()) <= kMaxAgeSecs;
    peak_ = valid_ ? solardrap::peak(grid_) : 0;
    if (valid_) {
        MapStyle style;
        style.id = "drap";
        style.name = "D-RAP";
        style.image = solardrap::to_image(grid_);
        style.opacity = 0.7;
        style.legend = solardrap::drap_scale();
        style.credit = "NOAA SWPC D-RAP";
        MapStyles::instance().set_style(style);
    } else {
        MapStyles::instance().remove_style("drap");  // never leave an old table on the map
    }
    emit valuesChanged();
}

void register_widget_solardrap(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SolarDrapModel(context);
    registry.add({"solardrap", "D-RAP", QUrl("qrc:/widgets/solardrap/SolarDrapCard.qml"), "strip", 58}, model);
    QTimer::singleShot(0, model, &SolarDrapModel::start);
}

} // namespace hamclock
