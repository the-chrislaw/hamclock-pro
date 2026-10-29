#include "sensors_model.hpp"
#include "sensors_read.hpp"
#include "../../ui/app_settings.hpp"
#include <QSettings>
#include <cmath>

namespace hamclock {

SensorsModel::SensorsModel(const WidgetContext& context, QString thermal_root, QString wireless_path,
                           QString disk_path, QString iio_root,
                           std::function<std::optional<platform::ClockSync>()> clock_reader, QObject* parent)
    : WidgetModel("sensors", "Host sensors", parent), root_(std::move(thermal_root)),
      wireless_(std::move(wireless_path)), diskPath_(std::move(disk_path)), iio_(std::move(iio_root)),
      clockReader_(std::move(clock_reader)), settings_(context.settings) {
    timer_.setInterval(10000);
    connect(&timer_, &QTimer::timeout, this, &SensorsModel::refresh);
}

void SensorsModel::start() { refresh(); timer_.start(); }

void SensorsModel::refresh() {
    temp_ = sensors::read_cpu_temp(root_);
    if (temp_) cpuPeak_.add(now().toSecsSinceEpoch(), *temp_);
    wifi_ = sensors::read_wifi_dbm(wireless_);
    if (wifi_) wifiHistory_.add(now().toSecsSinceEpoch(), *wifi_);
    disk_ = sensors::read_disk_used_fraction(diskPath_);
    diskFree_ = disk_ ? sensors::read_disk_free_text(diskPath_) : QString();
    env_ = sensors::read_env_sensor(iio_);
    if (settings_ && env_.any()) {
        // Optional corrections for a sensor that reads off: [sensors] temp_offset_c / pressure_offset_hpa.
        QSettings ini(settings_->optionsPath(), QSettings::IniFormat);
        sensors::apply_offsets(env_, ini.value("sensors/temp_offset_c").toString(),
                               ini.value("sensors/pressure_offset_hpa").toString());
    }
    if (env_.temp_c) envTempHistory_.add(now().toSecsSinceEpoch(), *env_.temp_c);
    if (env_.pressure_hpa) pressure_.add(now().toSecsSinceEpoch(), *env_.pressure_hpa);
    clock_ = clockReader_ ? clockReader_() : std::nullopt;
    if (temp_ || wifi_ || disk_ || env_.any() || clock_) setState("live", "Host sensors", now());
    else setState("unconfigured", "No host sensors available on this device");
    emit changed();
}

QString SensorsModel::cpuTemp() const {
    return temp_ ? QString("%1 °C").arg(std::lround(*temp_)) : QString();
}

QString SensorsModel::wifi() const {
    return wifi_ ? QString("%1 dBm").arg(*wifi_) : QString();
}

QString SensorsModel::wifiLow() const {
    const auto p = wifiHistory_.trough();
    return p ? QString("Weakest %1 dBm (3 h)").arg(std::lround(*p)) : QString();
}

QString SensorsModel::disk() const {
    return disk_ ? QString("%1% used").arg(std::lround(*disk_ * 100)) : QString();
}

QString SensorsModel::envTemp() const {
    return env_.temp_c ? QString("%1 °C").arg(*env_.temp_c, 0, 'f', 1) : QString();
}
QString SensorsModel::envPressure() const {
    return env_.pressure_hpa ? QString("%1 hPa").arg(std::lround(*env_.pressure_hpa)) : QString();
}
QString SensorsModel::envHumidity() const {
    return env_.humidity_pct ? QString("%1%").arg(std::lround(*env_.humidity_pct)) : QString();
}
QString SensorsModel::envDewPoint() const {
    if (!env_.temp_c || !env_.humidity_pct) return QString();
    const auto d = sensors::dew_point_c(*env_.temp_c, *env_.humidity_pct);
    return d ? QString("%1 °C").arg(*d, 0, 'f', 1) : QString();
}

QString SensorsModel::cpuNote() const {
    if (!temp_) return QString();
    return *temp_ >= 80 ? "Hot: throttling likely" : *temp_ >= 70 ? "Warm" : "Normal";
}

QString SensorsModel::cpuPeak() const {
    const auto p = cpuPeak_.peak();
    return p ? QString("Peak %1 °C (3 h)").arg(std::lround(*p)) : QString();
}

void register_widget_sensors(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SensorsModel(context);
    // Not listed on iOS (no local station hardware there): never started.
    if (registry.add({"sensors", "Host sensors", QUrl("qrc:/widgets/sensors/SensorsCard.qml"), "strip", 70}, model)) QTimer::singleShot(0, model, &SensorsModel::start);
}

} // namespace hamclock
