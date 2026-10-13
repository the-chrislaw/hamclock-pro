#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../../domain/astro_satellite.hpp"
#include <QStringList>
#include <QTimer>
#include <QVariantList>

namespace hamclock {

class PolledSource;
class AppSettings;

// Next amateur-satellite passes at the DE station: CelesTrak TLEs (cached,
// refreshed daily) propagated offline with SGP4 from the system clock.
class SatellitesModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY changed)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
public:
    explicit SatellitesModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();
    void recompute();

    bool hasData() const { return !tles_.empty(); }
    // Soonest first: {name, when ("now" / "in 42 min" / "14:05Z"), maxEl, label}.
    QVariantList rows() const { return rows_; }
    PolledSource& source() { return *source_; }

signals:
    void changed();

private:
    void load();

    AppSettings* settings_;
    PolledSource* source_;
    std::vector<Tle> tles_;
    QTimer timer_;
    QVariantList rows_;
};

// Short display name: the bracketed designator ("AO-7") if present, else the trimmed name.
QString satellite_short_name(const QString& tle_name);

void register_widget_satellites(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
