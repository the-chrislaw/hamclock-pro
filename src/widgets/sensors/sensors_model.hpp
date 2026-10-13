#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include <optional>

namespace hamclock {

// Host health: CPU temperature (sysfs). Shows "unconfigured" with a plain
// message on machines without a thermal zone (macOS, iPad, containers).
class SensorsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasCpuTemp READ hasCpuTemp NOTIFY changed)
    Q_PROPERTY(QString cpuTemp READ cpuTemp NOTIFY changed)
    Q_PROPERTY(QString cpuNote READ cpuNote NOTIFY changed)
public:
    explicit SensorsModel(const WidgetContext& context, QString thermal_root = "/sys/class/thermal",
                          QObject* parent = nullptr);
    void start();
    bool hasCpuTemp() const { return temp_.has_value(); }
    QString cpuTemp() const;
    QString cpuNote() const;
public slots:
    void refresh();
signals:
    void changed();
private:
    QString root_;
    QTimer timer_;
    std::optional<double> temp_;
};

void register_widget_sensors(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
