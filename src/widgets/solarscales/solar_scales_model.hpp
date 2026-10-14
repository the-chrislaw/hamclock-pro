#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "solar_scales_parse.hpp"
#include <QVariantList>

namespace hamclock {

// NOAA R (radio blackout), S (solar radiation) and G (geomagnetic) scales:
// current level, 24 h maximum and a three-day outlook.
class SolarScalesModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList scales READ scales NOTIFY valuesChanged)
    Q_PROPERTY(QString observed READ observed NOTIFY valuesChanged)
public:
    explicit SolarScalesModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return data_.valid; }
    // Three entries R, S, G: {letter, name, level, word, max24, outlook ("R1-2 10%  R3+ 1%" / "G1 expected")}.
    QVariantList scales() const;
    QString observed() const;  // "02:09Z" time of the current record, empty without data
    PolledSource& source() { return *source_; }

signals:
    void valuesChanged();

private:
    void parse();

    PolledSource* source_;
    solarscales::Scales data_;
};

void register_widget_solarscales(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
