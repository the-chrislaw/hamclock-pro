#pragma once
// Station header data for QML: UTC and DE local clocks, DE/DX grids, path.
#include "banner_text.hpp"
#include "path_text.hpp"
#include "sun_text.hpp"
#include <QObject>
#include <QString>
#include <QTimer>

namespace hamclock {

class AppSettings;

class StationModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString utcTime READ utcTime NOTIFY tick)
    Q_PROPERTY(QString utcDate READ utcDate NOTIFY tick)
    Q_PROPERTY(QString deTime READ deTime NOTIFY tick)
    Q_PROPERTY(QString deClock READ deClock NOTIFY tick)
    Q_PROPERTY(QString deDate READ deDate NOTIFY tick)
    Q_PROPERTY(QString deZoneLabel READ deZoneLabel NOTIFY tick)
    Q_PROPERTY(QString callsign READ callsign NOTIFY stationChanged)
    Q_PROPERTY(QString deGrid READ deGrid NOTIFY stationChanged)
    Q_PROPERTY(QString dxGrid READ dxGrid NOTIFY stationChanged)
    Q_PROPERTY(QString deCoordinates READ deCoordinates NOTIFY stationChanged)
    Q_PROPERTY(QString dxCoordinates READ dxCoordinates NOTIFY stationChanged)
    Q_PROPERTY(QString distanceText READ distanceText NOTIFY stationChanged)
    Q_PROPERTY(QString pathText READ pathText NOTIFY stationChanged)
    Q_PROPERTY(QString bannerText READ bannerText NOTIFY stationChanged)
    Q_PROPERTY(bool bannerIsCallsign READ bannerIsCallsign NOTIFY stationChanged)
    Q_PROPERTY(QString auxTime READ auxTime NOTIFY tick)
    Q_PROPERTY(bool longPath READ longPath NOTIFY stationChanged)
    Q_PROPERTY(QString bearingText READ bearingText NOTIFY stationChanged)
    Q_PROPERTY(QString deSun READ deSun NOTIFY tick)
    Q_PROPERTY(QString dxSun READ dxSun NOTIFY tick)
    Q_PROPERTY(bool configured READ configured NOTIFY stationChanged)
public:
    explicit StationModel(AppSettings& settings, QObject* parent = nullptr);

    QString utcTime() const;
    QString utcDate() const;
    QString deTime() const;      // empty when no valid DE zone
    QString deClock() const;     // HH:mm:ss at DE, empty when no valid zone
    QString deDate() const;
    QString deZoneLabel() const;
    QString callsign() const { return callsign_; }
    QString deGrid() const { return de_grid_; }
    QString dxGrid() const { return dx_grid_; }
    QString deCoordinates() const { return de_coordinates_; }
    QString dxCoordinates() const { return dx_coordinates_; }
    QString distanceText() const { return distance_; }
    QString bearingText() const { return bearing_; }
    QString pathText() const;
    bool longPath() const { return long_path_; }
    Q_INVOKABLE void toggleLongPath();
    Q_INVOKABLE void cycleBanner();   // call -> ON AIR -> 73 DE call (per session)
    QString bannerText() const;
    bool bannerIsCallsign() const { return banner_is_callsign(banner_mode_); }
    Q_INVOKABLE void cycleAuxTime();  // day of year -> JD -> MJD -> LST -> UNIX (per session)
    QString auxTime() const;          // LST uses the DE longitude, Greenwich when unset
    QString deSun() const;       // next sunrise/sunset at DE, empty when unconfigured
    QString dxSun() const;
    bool configured() const;

signals:
    void tick();
    void stationChanged();

private:
    void reload();
    AppSettings& settings_;
    QTimer timer_;
    bool long_path_ = false;
    bool has_geometry_ = false;
    GeometryResult geometry_{};
    double de_lat_ = 0, de_lon_ = 0, dx_lat_ = 0, dx_lon_ = 0;
    int banner_mode_ = 0;
    int aux_time_mode_ = 0;
    QString callsign_, de_grid_, dx_grid_, de_coordinates_, dx_coordinates_, distance_, bearing_;
};

} // namespace hamclock
