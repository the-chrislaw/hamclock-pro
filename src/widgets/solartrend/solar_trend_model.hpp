#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "solar_trend_parse.hpp"
#include <QVariantList>

namespace hamclock {

// Longer-term space weather from NOAA SWPC: 3-day Kp bars (observed + forecast),
// 30-day 10.7 cm flux trend and the latest monthly sunspot number. Values are
// "—" until received.
class SolarTrendModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasSunspots READ hasSunspots NOTIFY valuesChanged)
    Q_PROPERTY(double sunspots READ sunspots NOTIFY valuesChanged)
    Q_PROPERTY(QString sunspotMonth READ sunspotMonth NOTIFY valuesChanged)
    Q_PROPERTY(bool hasFlux READ hasFlux NOTIFY valuesChanged)
    Q_PROPERTY(int fluxLatest READ fluxLatest NOTIFY valuesChanged)
    Q_PROPERTY(QString fluxCaption READ fluxCaption NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList fluxHistory READ fluxHistory NOTIFY valuesChanged)
    Q_PROPERTY(bool hasKp READ hasKp NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList kpBars READ kpBars NOTIFY valuesChanged)
public:
    explicit SolarTrendModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasSunspots() const { return has_ssn_; }
    double sunspots() const { return ssn_.number; }
    QString sunspotMonth() const;  // e.g. "Sep 2026"
    bool hasFlux() const { return !flux_.empty(); }
    int fluxLatest() const { return flux_.empty() ? 0 : qRound(flux_.back().flux); }
    QString fluxCaption() const;   // "+3 vs 30 days ago" style
    QVariantList fluxHistory() const;
    bool hasKp() const { return !kp_.empty(); }
    // Each entry: {kp, predicted, level} oldest first.
    QVariantList kpBars() const;

    PolledSource& kpSource() { return *kp_source_; }
    PolledSource& fluxSource() { return *flux_source_; }
    PolledSource& sunspotSource() { return *ssn_source_; }

signals:
    void valuesChanged();

private:
    void parse();
    void refreshState();

    PolledSource* kp_source_;
    PolledSource* flux_source_;
    PolledSource* ssn_source_;
    std::vector<solartrend::KpBar> kp_;
    std::vector<solartrend::FluxDay> flux_;
    solartrend::Sunspots ssn_;
    bool has_ssn_ = false;
};

void register_widget_solartrend(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
