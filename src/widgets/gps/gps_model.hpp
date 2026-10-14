#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "gps_parse.hpp"
#include <QTcpSocket>
#include <QTimer>

namespace hamclock {

// Read-only gpsd client (localhost by default). Shows fix quality, position,
// Maidenhead grid and satellites in use. Without gpsd it reports
// "unconfigured"; it never changes the station location by itself.
class GpsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasFix READ hasFix NOTIFY changed)
    Q_PROPERTY(QString fixText READ fixText NOTIFY changed)
    Q_PROPERTY(QString position READ position NOTIFY changed)
    Q_PROPERTY(QString grid READ grid NOTIFY changed)
    Q_PROPERTY(QString satellites READ satellites NOTIFY changed)
public:
    explicit GpsModel(const WidgetContext& context, QString host = "127.0.0.1", quint16 port = 2947, QObject* parent = nullptr);
    void start();
    bool hasFix() const { return fix_.has_position(); }
    QString fixText() const;
    QString position() const;
    QString grid() const;
    QString satellites() const;
signals:
    void changed();
private slots:
    void connectNow();
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onSuspended();
    void onResumed();
private:
    void apply(const QByteArray& line);
    QString host_;
    quint16 port_;
    QTcpSocket socket_;
    bool started_ = false;
    QTimer retry_;
    QByteArray buffer_;
    gps::Fix fix_;
    gps::Sky sky_;
    bool haveTpv_ = false;
};

void register_widget_gps(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
