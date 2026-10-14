#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../solarscales/solar_scales_parse.hpp"
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
    // Three entries worst first: {letter, level, word, max24, effect}. Ties: higher 24 h max, then R, S, G.
    QVariantList ranked() const;
    PolledSource& source() { return *source_; }

signals:
    void valuesChanged();

private:
    void parse();

    PolledSource* source_;
    solarscales::Scales data_;
};

// Effect text for a level (0-5) of scale 'R', 'S' or 'G'; empty if out of range.
QString impact_effect(char letter, int level);

void register_widget_solarimpact(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
