#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include <QDateTime>
#include <QVariantList>
#include <memory>

namespace hamclock {

// Planetary Kp, 10.7 cm solar flux and GOES X-ray class from NOAA SWPC.
// Each value carries its own observation time; the card shows "—" for any
// value not yet received rather than a placeholder number.
class SpaceWeatherModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasKp READ hasKp NOTIFY valuesChanged)
    Q_PROPERTY(double kp READ kp NOTIFY valuesChanged)
    Q_PROPERTY(QString kpText READ kpText NOTIFY valuesChanged)
    Q_PROPERTY(bool hasSolarFlux READ hasSolarFlux NOTIFY valuesChanged)
    Q_PROPERTY(int solarFlux READ solarFlux NOTIFY valuesChanged)
    Q_PROPERTY(QString xrayClass READ xrayClass NOTIFY valuesChanged)
    Q_PROPERTY(bool hasXrayHistory READ hasXrayHistory NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList xrayHistory READ xrayHistory NOTIFY valuesChanged)
public:
    explicit SpaceWeatherModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasKp() const { return has_kp_; }
    double kp() const { return kp_; }
    QString kpText() const;
    bool hasSolarFlux() const { return has_sfi_; }
    int solarFlux() const { return sfi_; }
    QString xrayClass() const { return xray_class_; }

    // 24 h X-ray history: plot positions 0..1 (A1 to X10, log scale), oldest first,
    // one point per 10 min. Empty until received or once the data is out of date.
    bool hasXrayHistory() const { return history_.size() >= 2; }
    QVariantList xrayHistory() const { return history_; }

    // For tests: the three underlying sources.
    PolledSource& kpSource() { return *kp_source_; }
    PolledSource& fluxSource() { return *flux_source_; }
    PolledSource& xraySource() { return *xray_source_; }
    PolledSource& xrayDaySource() { return *xray_day_source_; }

signals:
    void valuesChanged();

private:
    void parse();
    void refreshState();

    PolledSource* kp_source_;
    PolledSource* flux_source_;
    PolledSource* xray_source_;
    PolledSource* xray_day_source_;
    QVariantList history_;
    bool has_kp_ = false, has_sfi_ = false;
    double kp_ = 0;
    int sfi_ = 0;
    QString xray_class_;
};

void register_widget_spaceweather(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
