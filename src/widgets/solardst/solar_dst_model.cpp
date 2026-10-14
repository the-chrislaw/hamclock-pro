#include "solar_dst_model.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>
#include <algorithm>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
constexpr PollPolicy kPolicy{30min, 2h, 1h};
constexpr char kUrl[] = "https://services.swpc.noaa.gov/products/kyoto-dst.json";
constexpr int kMaxAgeSecs = 6 * 3600;
} // namespace

SolarDstModel::SolarDstModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solardst", "Dst index", parent),
      source_(new PolledSource("swpc-kyoto-dst", QUrl(kUrl), kPolicy, context.transport, this)) {
    trackSource(*source_, "NOAA SWPC / Kyoto WDC");
    connect(source_, &PolledSource::updated, this, &SolarDstModel::parse);
}

void SolarDstModel::start() { source_->start(); }

void SolarDstModel::parse() {
    samples_ = solardst::parse_dst(source_->body());
    valid_ = !samples_.empty() && samples_.back().time.secsTo(now()) <= kMaxAgeSecs;
    emit valuesChanged();
}

int SolarDstModel::dst() const { return valid_ ? samples_.back().dst : 0; }
QString SolarDstModel::word() const { return valid_ ? solardst::dst_word(samples_.back().dst) : QString(); }

int SolarDstModel::min24() const {
    if (!valid_) return 0;
    const auto from = samples_.back().time.addSecs(-24 * 3600);
    int lo = samples_.back().dst;
    for (const auto& s : samples_)
        if (s.time >= from) lo = std::min(lo, s.dst);
    return lo;
}

QString SolarDstModel::observed() const { return valid_ ? samples_.back().time.toString("HH:mm") + "Z" : QString(); }

QVariantList SolarDstModel::history() const {
    QVariantList out;
    if (!valid_) return out;
    const auto from = samples_.back().time.addSecs(-48 * 3600);
    for (const auto& s : samples_)
        if (s.time >= from) out.push_back(s.dst);
    return out;
}

void register_widget_solardst(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SolarDstModel(context);
    registry.add({"solardst", "Dst index", QUrl("qrc:/widgets/solardst/SolarDstCard.qml"), "strip", 56}, model);
    QTimer::singleShot(0, model, &SolarDstModel::start);
}

} // namespace hamclock
