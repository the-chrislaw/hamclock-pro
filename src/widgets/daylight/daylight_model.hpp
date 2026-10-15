#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../../domain/astro_sun.hpp"
#include <QTimer>
#include <QVariantList>

namespace hamclock {

class AppSettings;

// Yearly sunrise/sunset chart data for DE (offline). Points are every
// `step_days` days: {day, rise, set} with rise/set in minutes after 00:00 UTC,
// or -1 when the sun does not rise/set that day (polar day or night).
// With a valid IANA `zone_id` the minutes are local time in that zone (DST from the OS tz
// database, offset taken at 12:00 UTC of each day); otherwise UTC.
QVariantList daylight_points(int year, double lat, double lon, int step_days, const QString& zone_id = {});

class DaylightModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(QVariantList points READ points NOTIFY changed)
    Q_PROPERTY(int today READ today NOTIFY changed)         // 0-based day of year
    Q_PROPERTY(int daysInYear READ daysInYear NOTIFY changed)
    Q_PROPERTY(bool hasStation READ hasStation NOTIFY changed)
    Q_PROPERTY(bool localTime READ localTime NOTIFY changed) // minutes are DE local time, not UTC
public:
    explicit DaylightModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();
    void recompute();
    QVariantList points() const { return points_; }
    int today() const { return today_; }
    int daysInYear() const { return days_; }
    bool hasStation() const { return has_station_; }
    bool localTime() const { return local_; }
signals:
    void changed();
private:
    AppSettings* settings_;
    QTimer timer_;
    bool has_station_ = false, local_ = false;
    int today_ = 0, days_ = 365;
    QVariantList points_;
};

void register_widget_daylight(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
