#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include <QTcpSocket>
#include <QTimer>
#include <optional>

namespace hamclock {

// Read-only rotctld client (Hamlib, localhost by default). Polls the position
// with the read-only "p" command only; no "P" (set position), stop, park or
// reset is ever sent. Without rotctld it reports "unconfigured".
class RotatorModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasRotator READ hasRotator NOTIFY changed)
    Q_PROPERTY(QString azimuth READ azimuth NOTIFY changed)
    Q_PROPERTY(QString elevation READ elevation NOTIFY changed)
public:
    explicit RotatorModel(const WidgetContext& context, QString host = "127.0.0.1", quint16 port = 4533, QObject* parent = nullptr);
    void start();
    // Settings changed the rotctld address: drop the current link and connect to the new one.
    void setEndpoint(const QString& host, quint16 port);
    bool hasRotator() const { return az_.has_value(); }
    QString azimuth() const;
    QString elevation() const;
signals:
    void changed();
private slots:
    void connectNow();
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onSuspended();
    void onResumed();
    void poll();
private:
    enum class Wait { Idle, Azimuth, Elevation };
    void handleLine(const QString& line);
    void fail(const QString& text);
    QString unreachableText() const;
    QString host_;
    quint16 port_;
    QTcpSocket socket_;
    bool started_ = false;
    QTimer retry_, poll_;
    QByteArray buffer_;
    Wait wait_ = Wait::Idle;
    std::optional<double> az_, el_, pendingAz_;
};

void register_widget_rotator(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
