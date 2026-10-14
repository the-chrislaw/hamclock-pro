#include "solar_impact_model.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>
#include <QVariantMap>
#include <algorithm>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
constexpr PollPolicy kPolicy{10min, 2h, 1h};
constexpr char kUrl[] = "https://services.swpc.noaa.gov/products/noaa-scales.json";

struct Row { char letter; int now; int max; };
} // namespace

QString impact_effect(char letter, int level) {
    static const char* const r[] = {"No HF impact", "Weak HF fades on the sunlit side",
                                    "Limited HF blackout on the sunlit side", "Wide-area HF blackout, about an hour",
                                    "HF blackout on the sunlit side for 1-2 h", "Complete HF blackout on the sunlit side"};
    static const char* const s[] = {"No radiation impact", "Minor polar HF absorption", "Small effects on polar HF",
                                    "Degraded HF on polar paths", "Polar HF blackout likely", "Complete polar HF blackout"};
    static const char* const g[] = {"Quiet geomagnetic field", "Aurora at high latitudes",
                                    "Aurora to about 55 deg; HF fades at high latitudes",
                                    "Aurora to about 50 deg; intermittent HF and GPS problems",
                                    "Aurora to about 45 deg; HF and GPS degraded",
                                    "Aurora to low latitudes; HF may fail for days"};
    if (level < 0 || level > 5) return {};
    switch (letter) {
    case 'R': return r[level];
    case 'S': return s[level];
    case 'G': return g[level];
    default: return {};
    }
}

SolarImpactModel::SolarImpactModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solarimpact", "Space weather impact", parent),
      source_(new PolledSource("swpc-impact-scales", QUrl(kUrl), kPolicy, context.transport, this)) {
    trackSource(*source_, "NOAA SWPC");
    connect(source_, &PolledSource::updated, this, &SolarImpactModel::parse);
}

void SolarImpactModel::start() { source_->start(); }

void SolarImpactModel::parse() {
    data_ = solarscales::parse_scales(source_->body());
    emit valuesChanged();
}

namespace {
std::vector<Row> ordered(const solarscales::Scales& d) {
    std::vector<Row> rows{{'R', d.current.r, d.past24h.r}, {'S', d.current.s, d.past24h.s}, {'G', d.current.g, d.past24h.g}};
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        return a.now != b.now ? a.now > b.now : a.max > b.max;
    });
    return rows;
}
} // namespace

QString SolarImpactModel::headline() const {
    if (!data_.valid) return {};
    const Row top = ordered(data_).front();
    if (top.now <= 0) return "All quiet";
    return QString("%1%2 %3: %4").arg(QChar(top.letter)).arg(top.now).arg(solarscales::scale_word(top.now),
                                                                         impact_effect(top.letter, top.now));
}

QVariantList SolarImpactModel::ranked() const {
    QVariantList out;
    if (!data_.valid) return out;
    for (const Row& row : ordered(data_))
        out.push_back(QVariantMap{{"letter", QString(QChar(row.letter))}, {"level", row.now},
                                  {"word", solarscales::scale_word(row.now)}, {"max24", row.max},
                                  {"effect", impact_effect(row.letter, row.now)}});
    return out;
}

void register_widget_solarimpact(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SolarImpactModel(context);
    registry.add({"solarimpact", "Space weather impact", QUrl("qrc:/widgets/solarimpact/SolarImpactCard.qml"), "strip", 57},
                 model);
    QTimer::singleShot(0, model, &SolarImpactModel::start);
}

} // namespace hamclock
