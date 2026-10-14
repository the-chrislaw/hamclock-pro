#include "solar_scales_model.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>
#include <QVariantMap>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
constexpr PollPolicy kPolicy{10min, 2h, 1h};
constexpr char kUrl[] = "https://services.swpc.noaa.gov/products/noaa-scales.json";

// Outlook text for tomorrow (forecast[1]); percentages come straight from SWPC.
QString outlook(char letter, const solarscales::Day& d) {
    switch (letter) {
    case 'R':
        if (d.r_minor < 0 && d.r_major < 0) return {};
        return QString("R1-2 %1%  R3+ %2%").arg(qMax(d.r_minor, 0)).arg(qMax(d.r_major, 0));
    case 'S':
        return d.s_prob < 0 ? QString() : QString("S1+ %1%").arg(d.s_prob);
    default:
        return d.g < 0 ? QString() : d.g == 0 ? QString("none expected") : QString("G%1 expected").arg(d.g);
    }
}
} // namespace

SolarScalesModel::SolarScalesModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solarscales", "NOAA scales", parent),
      source_(new PolledSource("swpc-noaa-scales", QUrl(kUrl), kPolicy, context.transport, this)) {
    trackSource(*source_, "NOAA SWPC");
    connect(source_, &PolledSource::updated, this, &SolarScalesModel::parse);
}

void SolarScalesModel::start() { source_->start(); }

void SolarScalesModel::parse() {
    data_ = solarscales::parse_scales(source_->body());
    emit valuesChanged();
}

QString SolarScalesModel::observed() const {
    return data_.valid ? data_.observed.toString("HH:mm") + "Z" : QString();
}

QVariantList SolarScalesModel::scales() const {
    QVariantList out;
    if (!data_.valid) return out;
    struct Row { char letter; const char* name; int now; int max; };
    const Row rows[] = {{'R', "Radio blackouts", data_.current.r, data_.past24h.r},
                        {'S', "Radiation storms", data_.current.s, data_.past24h.s},
                        {'G', "Geomagnetic storms", data_.current.g, data_.past24h.g}};
    for (const auto& row : rows)
        out.push_back(QVariantMap{
            {"letter", QString(QChar(row.letter))}, {"name", row.name}, {"level", row.now},
            {"word", solarscales::scale_word(row.now)}, {"max24", row.max},
            {"outlook", outlook(row.letter, data_.forecast[1])}});
    return out;
}

void register_widget_solarscales(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SolarScalesModel(context);
    registry.add({"solarscales", "NOAA scales", QUrl("qrc:/widgets/solarscales/SolarScalesCard.qml"), "strip", 56},
                 model);
    QTimer::singleShot(0, model, &SolarScalesModel::start);
}

} // namespace hamclock
