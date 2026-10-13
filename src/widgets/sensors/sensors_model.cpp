#include "sensors_model.hpp"
#include "sensors_read.hpp"
#include <cmath>

namespace hamclock {

SensorsModel::SensorsModel(const WidgetContext&, QString thermal_root, QString wireless_path,
                           QString disk_path, QObject* parent)
    : WidgetModel("sensors", "Host sensors", parent), root_(std::move(thermal_root)),
      wireless_(std::move(wireless_path)), diskPath_(std::move(disk_path)) {
    timer_.setInterval(10000);
    connect(&timer_, &QTimer::timeout, this, &SensorsModel::refresh);
}

void SensorsModel::start() { refresh(); timer_.start(); }

void SensorsModel::refresh() {
    temp_ = sensors::read_cpu_temp(root_);
    wifi_ = sensors::read_wifi_dbm(wireless_);
    disk_ = sensors::read_disk_used_fraction(diskPath_);
    if (temp_ || wifi_ || disk_) setState("live", "Host sensors", now());
    else setState("unconfigured", "No host sensors available on this device");
    emit changed();
}

QString SensorsModel::cpuTemp() const {
    return temp_ ? QString("%1 °C").arg(std::lround(*temp_)) : QString();
}

QString SensorsModel::wifi() const {
    return wifi_ ? QString("%1 dBm").arg(*wifi_) : QString();
}

QString SensorsModel::disk() const {
    return disk_ ? QString("%1% used").arg(std::lround(*disk_ * 100)) : QString();
}

QString SensorsModel::cpuNote() const {
    if (!temp_) return QString();
    return *temp_ >= 80 ? "Hot: throttling likely" : *temp_ >= 70 ? "Warm" : "Normal";
}

void register_widget_sensors(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SensorsModel(context);
    registry.add({"sensors", "Host sensors", QUrl("qrc:/widgets/sensors/SensorsCard.qml"), "strip", 70}, model);
    QTimer::singleShot(0, model, &SensorsModel::start);
}

} // namespace hamclock
