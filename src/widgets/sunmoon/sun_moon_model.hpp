#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../../domain/astro_sun.hpp"
#include "../../domain/astro_moon.hpp"
#include <QString>
#include <QTimer>

namespace hamclock {

// Sunrise, sunset and day length at the DE and DX stations, computed offline
// from the system clock (no network). Times are UTC.
struct SunSummary {
    QString next_rise, next_set, day_length, note; // empty = not available
};
SunSummary summarize_sun(UtcSeconds now, double latitude_deg, double longitude_deg);

// Moon phase, next rise/set and the current az/el at one station, offline.
struct MoonSummary {
    QString phase, illumination, next_rise, next_set, position; // empty = not available
    double elongation_deg = 0; // Moon minus Sun ecliptic longitude, for drawing the disc
    bool southern = false;     // observer in the southern hemisphere: the lit side is mirrored
};
MoonSummary summarize_moon(UtcSeconds now, double latitude_deg, double longitude_deg);

class SunMoonModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasStation READ hasStation NOTIFY changed)
    Q_PROPERTY(QString deRise READ deRise NOTIFY changed)
    Q_PROPERTY(QString deSet READ deSet NOTIFY changed)
    Q_PROPERTY(QString deDay READ deDay NOTIFY changed)
    Q_PROPERTY(QString deNote READ deNote NOTIFY changed)
    Q_PROPERTY(QString dxRise READ dxRise NOTIFY changed)
    Q_PROPERTY(QString dxSet READ dxSet NOTIFY changed)
    Q_PROPERTY(QString dxDay READ dxDay NOTIFY changed)
    Q_PROPERTY(QString dxNote READ dxNote NOTIFY changed)
    Q_PROPERTY(QString moonPhase READ moonPhase NOTIFY changed)
    Q_PROPERTY(QString moonIllumination READ moonIllumination NOTIFY changed)
    Q_PROPERTY(QString moonRise READ moonRise NOTIFY changed)
    Q_PROPERTY(QString moonSet READ moonSet NOTIFY changed)
    Q_PROPERTY(QString moonPosition READ moonPosition NOTIFY changed)
    Q_PROPERTY(double moonElongation READ moonElongation NOTIFY changed)
    Q_PROPERTY(bool moonSouthern READ moonSouthern NOTIFY changed)
public:
    explicit SunMoonModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();
    void recompute();

    bool hasStation() const { return has_station_; }
    QString deRise() const { return de_.next_rise; }
    QString deSet() const { return de_.next_set; }
    QString deDay() const { return de_.day_length; }
    QString deNote() const { return de_.note; }
    QString dxRise() const { return dx_.next_rise; }
    QString dxSet() const { return dx_.next_set; }
    QString dxDay() const { return dx_.day_length; }
    QString dxNote() const { return dx_.note; }
    QString moonPhase() const { return moon_.phase; }
    QString moonIllumination() const { return moon_.illumination; }
    QString moonRise() const { return moon_.next_rise; }
    QString moonSet() const { return moon_.next_set; }
    QString moonPosition() const { return moon_.position; }
    double moonElongation() const { return moon_.elongation_deg; }
    bool moonSouthern() const { return moon_.southern; }

signals:
    void changed();

private:
    AppSettings* settings_;
    QTimer timer_;
    bool has_station_ = false;
    SunSummary de_, dx_;
    MoonSummary moon_; // as seen from DE
};

void register_widget_sunmoon(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
