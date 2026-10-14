#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../../domain/astro_eme.hpp"
#include <QString>
#include <QTimer>

namespace hamclock {

// EME planning from DE to DX, offline from the system clock: Moon az/el at
// both ends, the next window with the Moon above both horizons, and the
// distance with the path loss relative to perigee.
struct EmeSummary {
    QString de_position, dx_position, window, distance, degradation, declination; // empty = not available
    bool mutual_now = false;
};
EmeSummary summarize_eme(UtcSeconds now, double de_lat, double de_lon, double dx_lat, double dx_lon);

class EmeModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasStation READ hasStation NOTIFY changed)
    Q_PROPERTY(bool mutualNow READ mutualNow NOTIFY changed)
    Q_PROPERTY(QString dePosition READ dePosition NOTIFY changed)
    Q_PROPERTY(QString dxPosition READ dxPosition NOTIFY changed)
    Q_PROPERTY(QString window READ window NOTIFY changed)
    Q_PROPERTY(QString distance READ distance NOTIFY changed)
    Q_PROPERTY(QString degradation READ degradation NOTIFY changed)
    Q_PROPERTY(QString declination READ declination NOTIFY changed)
public:
    explicit EmeModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();
    void recompute();
    bool hasStation() const { return has_station_; }
    bool mutualNow() const { return s_.mutual_now; }
    QString dePosition() const { return s_.de_position; }
    QString dxPosition() const { return s_.dx_position; }
    QString window() const { return s_.window; }
    QString distance() const { return s_.distance; }
    QString degradation() const { return s_.degradation; }
    QString declination() const { return s_.declination; }
signals:
    void changed();
private:
    AppSettings* settings_;
    QTimer timer_;
    bool has_station_ = false;
    EmeSummary s_;
};

void register_widget_eme(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
