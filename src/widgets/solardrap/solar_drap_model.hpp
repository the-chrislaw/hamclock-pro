#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "solar_drap_parse.hpp"

namespace hamclock {

// D-RAP absorption: highest affected frequency, and the "drap" map style (published only
// while the table is recent; removed otherwise).
class SolarDrapModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY valuesChanged)
    Q_PROPERTY(double peakMhz READ peakMhz NOTIFY valuesChanged)
    Q_PROPERTY(QString peakText READ peakText NOTIFY valuesChanged)
    Q_PROPERTY(bool absorbing READ absorbing NOTIFY valuesChanged)
    Q_PROPERTY(QString message READ message NOTIFY valuesChanged)
    Q_PROPERTY(QString observed READ observed NOTIFY valuesChanged)
public:
    explicit SolarDrapModel(const WidgetContext& context, QObject* parent = nullptr);
    ~SolarDrapModel() override;
    void start();

    // False until a table valid at most 2 h ago is known.
    bool hasData() const { return valid_; }
    double peakMhz() const { return valid_ ? peak_ : 0; }
    QString peakText() const { return solardrap::peak_text(peakMhz()); }  // "Max 7.3 MHz" / "No HF absorption"
    bool absorbing() const { return peakMhz() >= solardrap::kMinAbsorbedMhz; }
    QString message() const;   // X-ray and proton status, empty without data
    QString observed() const;  // "06:07Z", empty without data
    PolledSource& source() { return *source_; }

signals:
    void valuesChanged();

private:
    void parse();

    PolledSource* source_;
    solardrap::Grid grid_;
    double peak_ = 0;
    bool valid_ = false;
};

void register_widget_solardrap(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
