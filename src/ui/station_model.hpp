#pragma once
// Station header data for QML: UTC and DE local clocks, DE/DX grids, path.
#include "path_text.hpp"
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
    Q_PROPERTY(QString bearingText READ bearingText NOTIFY stationChanged)
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
    QString pathText() const { return path_summary(distance_, bearing_); }
    bool configured() const;

signals:
    void tick();
    void stationChanged();

private:
    void reload();
    AppSettings& settings_;
    QTimer timer_;
    QString callsign_, de_grid_, dx_grid_, de_coordinates_, dx_coordinates_, distance_, bearing_;
};

} // namespace hamclock
