#include "solar_hamqsl_model.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>
#include <QVariantMap>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// N0NBH asks for at most one fetch per hour (flux updates hourly, the rest every 3 h).
constexpr PollPolicy kPolicy{1h, 3h, 2h};
constexpr char kUrl[] = "https://www.hamqsl.com/solarxml.php";
constexpr int kMaxAgeSecs = 6 * 3600;
} // namespace

SolarHamqslModel::SolarHamqslModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solarhamqsl", "HamQSL conditions", parent),
      source_(new PolledSource("hamqsl-solar", QUrl(kUrl), kPolicy, context.transport, this)) {
    trackSource(*source_, "HamQSL.com (N0NBH)");
    connect(source_, &PolledSource::updated, this, &SolarHamqslModel::parse);
}

void SolarHamqslModel::start() { source_->start(); }

void SolarHamqslModel::parse() {
    report_ = solarhamqsl::parse_report(source_->body());
    valid_ = report_.updated.isValid() && !report_.hf.empty() && report_.updated.secsTo(now()) <= kMaxAgeSecs;
    emit valuesChanged();
}

QVariantList SolarHamqslModel::bands() const {
    QVariantList out;
    if (!valid_) return out;
    for (const auto& b : report_.hf)
        out.push_back(QVariantMap{{"band", b.name}, {"day", b.day}, {"night", b.night},
                                  {"dayRank", solarhamqsl::condition_rank(b.day)},
                                  {"nightRank", solarhamqsl::condition_rank(b.night)}});
    return out;
}

QVariantList SolarHamqslModel::vhf() const {
    QVariantList out;
    if (!valid_) return out;
    for (const auto& v : report_.vhf)
        out.push_back(QVariantMap{{"name", v.name}, {"location", v.location}, {"status", v.status},
                                  {"open", v.status.compare("Band Closed", Qt::CaseInsensitive) != 0}});
    return out;
}

QString SolarHamqslModel::updated() const { return valid_ ? report_.updated.toString("HH:mm") + "Z" : QString(); }

void register_widget_solarhamqsl(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SolarHamqslModel(context);
    registry.add({"solarhamqsl", "HamQSL conditions", QUrl("qrc:/widgets/solarhamqsl/SolarHamqslCard.qml"), "strip", 56}, model);
    QTimer::singleShot(0, model, &SolarHamqslModel::start);
}

} // namespace hamclock
