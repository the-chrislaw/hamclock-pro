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
constexpr char kDstUrl[] = "https://services.swpc.noaa.gov/products/kyoto-dst.json";
constexpr PollPolicy kDstPolicy{30min, 6h, 3h};

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
    case 'D': {
        static const char* const d[] = {"Quiet ring current", "Weak storm", "Moderate storm; HF fades at high latitudes",
                                        "Intense storm; HF and GPS degraded", "", "Super storm; HF may fail for days"};
        return d[level];
    }
    default: return {};
    }
}

int dst_level(int dst) { return dst > -30 ? 0 : dst > -50 ? 1 : dst > -100 ? 2 : dst > -250 ? 3 : 5; }

SolarImpactModel::SolarImpactModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solarimpact", "Space weather impact", parent),
      source_(new PolledSource("swpc-impact-scales", QUrl(kUrl), kPolicy, context.transport, this)),
      dst_source_(new PolledSource("swpc-impact-dst", QUrl(kDstUrl), kDstPolicy, context.transport, this)) {
    trackSource(*source_, "NOAA SWPC");
    connect(source_, &PolledSource::updated, this, &SolarImpactModel::parse);
    connect(dst_source_, &PolledSource::updated, this, [this] {
        dst_ = solardst::parse_dst(dst_source_->body());
        emit valuesChanged();
    });
}

void SolarImpactModel::start() {
    source_->start();
    dst_source_->start();
}

void SolarImpactModel::parse() {
    data_ = solarscales::parse_scales(source_->body());
    emit valuesChanged();
}

namespace {
std::vector<Row> ordered(const solarscales::Scales& d, const std::vector<solardst::Sample>& dst, const QDateTime& now) {
    std::vector<Row> rows{{'R', d.current.r, d.past24h.r}, {'S', d.current.s, d.past24h.s}, {'G', d.current.g, d.past24h.g}};
    // Dst joins only while its latest hourly value is fresh; an old storm must not rank as current.
    if (!dst.empty() && dst.back().time.secsTo(now) < 6 * 3600) {
        int low = dst.back().dst;
        for (const auto& s : dst)
            if (s.time >= dst.back().time.addSecs(-24 * 3600)) low = std::min(low, s.dst);
        rows.push_back({'D', dst_level(dst.back().dst), dst_level(low)});
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        return a.now != b.now ? a.now > b.now : a.max > b.max;
    });
    return rows;
}
} // namespace

QString SolarImpactModel::headline() const {
    if (!data_.valid) return {};
    const Row top = ordered(data_, dst_, now()).front();
    if (top.now <= 0) return "All quiet";
    return QString("%1%2 %3: %4").arg(QChar(top.letter)).arg(top.now).arg(top.letter == 'D' ? solardst::dst_word(dst_.back().dst) : solarscales::scale_word(top.now),
                                                                         impact_effect(top.letter, top.now));
}

QVariantList SolarImpactModel::ranked() const {
    QVariantList out;
    if (!data_.valid) return out;
    for (const Row& row : ordered(data_, dst_, now()))
        out.push_back(QVariantMap{{"letter", QString(QChar(row.letter))}, {"level", row.now},
                                  {"word", row.letter == 'D' ? solardst::dst_word(dst_.back().dst) : solarscales::scale_word(row.now)}, {"max24", row.max},
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
