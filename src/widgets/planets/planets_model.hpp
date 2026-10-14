#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../../domain/astro_planets.hpp"
#include "../../map/overlay.hpp"
#include <QTimer>
#include <QVariantList>

namespace hamclock {

class AppSettings;

// The five naked-eye planets above the horizon at DE, brightest-first order of
// elevation (highest first), offline from the system clock. Rows are
// {name, position}; empty when none is up.
QVariantList planet_rows(UtcSeconds now, double de_lat, double de_lon);

// Map layer "planets": each planet at its sub-planet point (where it is overhead).
OverlayLayer planet_overlay_layer(UtcSeconds now);

class PlanetsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(bool hasStation READ hasStation NOTIFY changed)
public:
    explicit PlanetsModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();
    void recompute();
    QVariantList rows() const { return rows_; }
    bool hasStation() const { return has_station_; }
signals:
    void changed();
private:
    AppSettings* settings_;
    QTimer timer_;
    bool has_station_ = false;
    QVariantList rows_;
};

void register_widget_planets(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
