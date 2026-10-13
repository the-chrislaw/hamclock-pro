#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../../domain/astro_beacons.hpp"
#include <QTimer>
#include <QVariantList>

namespace hamclock {

// Which NCDXF/IARU beacon is on each of the five frequencies right now,
// from the fixed public schedule and the system clock (no network).
// One row per band: {band, call, location, remaining}.
QVariantList beacon_rows(UtcSeconds now);

class BeaconsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
public:
    explicit BeaconsModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();
    void recompute();
    QVariantList rows() const { return rows_; }
signals:
    void changed();
private:
    QTimer timer_;
    QVariantList rows_;
};

void register_widget_beacons(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
