#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "solar_alerts_parse.hpp"
#include <QVariantList>

namespace hamclock {

// NOAA SWPC space weather alerts, watches and warnings currently in force.
// An empty list from a fresh download is real data ("nothing in force"); with no
// data yet the card shows the loading/offline state instead.
class SolarAlertsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY valuesChanged)
    Q_PROPERTY(int count READ count NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList alerts READ alerts NOTIFY valuesChanged)
public:
    explicit SolarAlertsModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return has_data_; }
    int count() const { return static_cast<int>(active_.size()); }
    // Each entry: {kind, headline, scale, level, issued ("14:31Z"), until ("until 21:00Z" or "")}, newest first.
    QVariantList alerts() const;
    PolledSource& source() { return *source_; }

signals:
    void valuesChanged();

private:
    void parse();

    PolledSource* source_;
    std::vector<solaralerts::Alert> all_;
    std::vector<solaralerts::Alert> active_;
    bool has_data_ = false;
};

void register_widget_solaralerts(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
