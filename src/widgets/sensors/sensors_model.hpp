#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "sensors_read.hpp"
#include "../../platform/ntp_status.hpp"
#include <functional>
#include <optional>

namespace hamclock {

// Host health: CPU temperature (sysfs). Shows "unconfigured" with a plain
// message on machines without a thermal zone (macOS, iPad, containers).
class SensorsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasCpuTemp READ hasCpuTemp NOTIFY changed)
    Q_PROPERTY(QString cpuTemp READ cpuTemp NOTIFY changed)
    Q_PROPERTY(QString cpuNote READ cpuNote NOTIFY changed)
    Q_PROPERTY(bool hasWifi READ hasWifi NOTIFY changed)
    Q_PROPERTY(QString wifi READ wifi NOTIFY changed)
    Q_PROPERTY(bool hasEnv READ hasEnv NOTIFY changed)
    Q_PROPERTY(QString envTemp READ envTemp NOTIFY changed)
    Q_PROPERTY(QString envPressure READ envPressure NOTIFY changed)
    Q_PROPERTY(QString envTrend READ envTrend NOTIFY changed)
    Q_PROPERTY(QString envHumidity READ envHumidity NOTIFY changed)
    Q_PROPERTY(QString envDewPoint READ envDewPoint NOTIFY changed)
    Q_PROPERTY(bool hasClock READ hasClock NOTIFY changed)
    Q_PROPERTY(QString clock READ clock NOTIFY changed)
    Q_PROPERTY(bool hasDisk READ hasDisk NOTIFY changed)
    Q_PROPERTY(QString disk READ disk NOTIFY changed)
public:
    explicit SensorsModel(const WidgetContext& context, QString thermal_root = "/sys/class/thermal", QString wireless_path = "/proc/net/wireless",
                          QString disk_path = "/", QString iio_root = "/sys/bus/iio/devices",
                          std::function<std::optional<platform::ClockSync>()> clock_reader = platform::read_clock_sync,
                          QObject* parent = nullptr);
    void start();
    bool hasCpuTemp() const { return temp_.has_value(); }
    QString cpuTemp() const;
    QString cpuNote() const;
    bool hasWifi() const { return wifi_.has_value(); }
    QString wifi() const;
    bool hasEnv() const { return env_.any(); }
    QString envTemp() const;
    QString envPressure() const;
    QString envTrend() const { return pressure_.trend_word(); }
    QString envHumidity() const;
    QString envDewPoint() const;
    bool hasClock() const { return clock_.has_value(); }
    QString clock() const { return clock_ ? platform::describe(*clock_) : QString(); }
    bool hasDisk() const { return disk_.has_value(); }
    QString disk() const;
public slots:
    void refresh();
signals:
    void changed();
private:
    QString root_, wireless_, diskPath_, iio_;
    sensors::EnvReading env_;
    sensors::PressureHistory pressure_;
    std::function<std::optional<platform::ClockSync>()> clockReader_;
    std::optional<platform::ClockSync> clock_;
    QTimer timer_;
    std::optional<double> temp_, disk_;
    std::optional<int> wifi_;
};

void register_widget_sensors(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
