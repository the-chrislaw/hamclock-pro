#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "gps_nmea.hpp"
#include "gps_parse.hpp"
#include "gps_serial.hpp"
#include <QTcpSocket>
#include <QTimer>

namespace hamclock {

// Read-only gpsd client (localhost by default). Shows fix quality, position,
// Maidenhead grid and satellites in use. Without gpsd it reports
// "unconfigured"; it never changes the station location by itself.
// With [gps] device set in options.ini it reads that serial NMEA receiver instead.
class GpsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasFix READ hasFix NOTIFY changed)
    Q_PROPERTY(QString fixText READ fixText NOTIFY changed)
    Q_PROPERTY(QString position READ position NOTIFY changed)
    Q_PROPERTY(QString grid READ grid NOTIFY changed)
    Q_PROPERTY(QString clockCheck READ clockCheck NOTIFY changed)
    Q_PROPERTY(QString satellites READ satellites NOTIFY changed)
public:
    explicit GpsModel(const WidgetContext& context, QString host = "127.0.0.1", quint16 port = 2947, QObject* parent = nullptr);
    void start();
    // Serial NMEA receiver instead of gpsd (read-only). Call before start().
    void useSerial(const gps::SerialConfig& cfg) { serial_cfg_ = cfg; setState("unconfigured", unreachableText()); }
    // Settings changed the source: close the current gpsd link or serial device and open the new
    // one (an empty serial device means gpsd at host:port).
    void setSource(const QString& host, quint16 port, const gps::SerialConfig& cfg);
    bool hasFix() const { return fix_.has_position(); }
    QString fixText() const;
    QString position() const;
    QString grid() const;
    QString satellites() const;
    QString clockCheck() const;  // empty without a fresh GPS time
signals:
    void changed();
private slots:
    void connectNow();
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onSuspended();
    void onResumed();
    void onSerialData(const QByteArray& chunk);
    void refreshSerial();
private:
    void apply(const QByteArray& line);
    QString unreachableText() const;
    QString host_;
    quint16 port_;
    QTcpSocket socket_;
    bool started_ = false;
    QTimer retry_;
    QByteArray buffer_;
    gps::Fix fix_;
    gps::Sky sky_;
    bool haveTpv_ = false;
    gps::SerialConfig serial_cfg_;
    gps::SerialPort serial_;
    gps::NmeaStream nmea_stream_;
    gps::NmeaTracker nmea_;
    QTimer serial_tick_;
    bool serial_seen_ = false;
};

void register_widget_gps(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
