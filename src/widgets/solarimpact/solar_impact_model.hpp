#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../solarscales/solar_scales_parse.hpp"
#include "../solardst/solar_dst_parse.hpp"
#include <QVariantList>

namespace hamclock {

// Space-weather conditions ranked by operating impact: the NOAA R (radio
// blackout), S (radiation) and G (geomagnetic) levels, worst first, each with
// a one-line effect on HF/aurora/GPS taken from the NOAA scale definitions.
class SolarImpactModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY valuesChanged)
    Q_PROPERTY(QString headline READ headline NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList ranked READ ranked NOTIFY valuesChanged)
public:
    explicit SolarImpactModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return data_.valid; }
    // "All quiet" or "G3 Strong: <effect>"; empty without data.
    QString headline() const;
    // Three entries (R, S, G) plus a fourth {letter "D"} for the Kyoto Dst index while its latest hourly value is under 6 h old,
    // worst first: {letter, level, word, max24, effect}. Dst levels: Quiet 0, Weak 1, Moderate 2, Intense 3, Super 5; max24 is the 24 h low level. Ties: higher 24 h max, then R, S, G.
    QVariantList ranked() const;
    PolledSource& source() { return *source_; }
    PolledSource& dstSource() { return *dst_source_; }

signals:
    void valuesChanged();

private:
    void parse();

    PolledSource* source_;
    PolledSource* dst_source_;
    solarscales::Scales data_;
    std::vector<solardst::Sample> dst_;
};

// Effect text for a level (0-5) of scale 'R', 'S' or 'G'; empty if out of range.
QString impact_effect(char letter, int level);
// Impact level (0-5) for a Dst value in nT, comparable to the NOAA scales.
int dst_level(int dst);

void register_widget_solarimpact(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
