#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "solar_aurora_parse.hpp"

namespace hamclock {

// OVATION aurora nowcast: peak probability per hemisphere, and the "aurora" map style
// (published only while the grid is recent; removed otherwise).
class SolarAuroraModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY valuesChanged)
    Q_PROPERTY(int northPeak READ northPeak NOTIFY valuesChanged)
    Q_PROPERTY(int southPeak READ southPeak NOTIFY valuesChanged)
    Q_PROPERTY(QString observed READ observed NOTIFY valuesChanged)
public:
    explicit SolarAuroraModel(const WidgetContext& context, QObject* parent = nullptr);
    ~SolarAuroraModel() override;
    void start();

    // False until a grid observed at most 3 h ago is known.
    bool hasData() const { return valid_; }
    int northPeak() const { return valid_ ? north_ : 0; }
    int southPeak() const { return valid_ ? south_ : 0; }
    QString observed() const;  // "04:57Z", empty without data
    PolledSource& source() { return *source_; }

signals:
    void valuesChanged();

private:
    void parse();

    PolledSource* source_;
    solaraurora::Grid grid_;
    int north_ = 0, south_ = 0;
    bool valid_ = false;
};

void register_widget_solaraurora(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
