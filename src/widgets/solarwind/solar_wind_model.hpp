#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "solar_wind_parse.hpp"
#include <QVariantList>
#include <vector>

namespace hamclock {

// Real-time solar wind at L1 (NOAA SWPC): speed, density, IMF Bt and Bz, each
// with the last hour or so as sparkline history. Values are "—" until received.
class SolarWindModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasSpeed READ hasSpeed NOTIFY valuesChanged)
    Q_PROPERTY(double speed READ speed NOTIFY valuesChanged)
    Q_PROPERTY(QString speedText READ speedText NOTIFY valuesChanged)
    Q_PROPERTY(bool hasDensity READ hasDensity NOTIFY valuesChanged)
    Q_PROPERTY(double density READ density NOTIFY valuesChanged)
    Q_PROPERTY(bool hasBz READ hasBz NOTIFY valuesChanged)
    Q_PROPERTY(double bz READ bz NOTIFY valuesChanged)
    Q_PROPERTY(QString bzText READ bzText NOTIFY valuesChanged)
    Q_PROPERTY(bool hasBt READ hasBt NOTIFY valuesChanged)
    Q_PROPERTY(double bt READ bt NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList speedHistory READ speedHistory NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList bzHistory READ bzHistory NOTIFY valuesChanged)
public:
    explicit SolarWindModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasSpeed() const { return !speed_.empty(); }
    double speed() const { return speed_.empty() ? 0 : speed_.back().value; }
    QString speedText() const;
    bool hasDensity() const { return !density_.empty(); }
    double density() const { return density_.empty() ? 0 : density_.back().value; }
    bool hasBz() const { return !bz_.empty(); }
    double bz() const { return bz_.empty() ? 0 : bz_.back().value; }
    QString bzText() const;
    bool hasBt() const { return !bt_.empty(); }
    double bt() const { return bt_.empty() ? 0 : bt_.back().value; }
    QVariantList speedHistory() const { return toList(speed_); }
    QVariantList bzHistory() const { return toList(bz_); }

    PolledSource& windSource() { return *wind_source_; }
    PolledSource& magSource() { return *mag_source_; }

signals:
    void valuesChanged();

private:
    using Series = std::vector<solarwind::Point>;
    static QVariantList toList(const Series& series);
    static Series lastHour(Series series);
    void parse();
    void refreshState();

    PolledSource* wind_source_;
    PolledSource* mag_source_;
    Series speed_, density_, bz_, bt_;
};

void register_widget_solarwind(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
