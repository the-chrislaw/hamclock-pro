#include "view_options.hpp"
#include <QSettings>
#include <algorithm>
namespace hamclock {
void MapViewOptions::set_layer_visible(const QString& id, bool on) {
    if (on) hidden_layers.remove(id); else hidden_layers.insert(id);
}
std::vector<OverlayLayer> MapViewOptions::apply(const std::vector<OverlayLayer>& layers) const {
    auto out = layers;
    for (auto& l : out) {
        if (hidden_layers.contains(l.id)) l.visible = false;
        if (label_mode >= 0) l.labels = static_cast<OverlayLabelMode>(label_mode);
    }
    return out;
}
void MapViewOptions::load(const QString& ini_path) {
    QSettings ini(ini_path, QSettings::IniFormat);
    hidden_layers.clear();
    for (const auto& id : ini.value("map/hidden_layers").toStringList())
        if (!id.isEmpty() && id.size() <= 64) hidden_layers.insert(id);
    const int mode = ini.value("map/labels", -1).toInt();
    label_mode = (mode >= 0 && mode <= static_cast<int>(OverlayLabelMode::Dot)) ? mode : -1;
    night_shade = ini.value("map/night", true).toBool();
    path = ini.value("map/path", true).toBool();
}
bool MapViewOptions::save(const QString& ini_path) const {
    QSettings ini(ini_path, QSettings::IniFormat);
    QStringList ids(hidden_layers.begin(), hidden_layers.end());
    std::sort(ids.begin(), ids.end());
    ini.setValue("map/hidden_layers", ids);
    ini.setValue("map/labels", label_mode);
    ini.setValue("map/night", night_shade);
    ini.setValue("map/path", path);
    ini.sync();
    return ini.status() == QSettings::NoError;
}
}
