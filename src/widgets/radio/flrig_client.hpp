#pragma once
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

namespace hamclock {

// Read-only flrig XML-RPC poller (localhost). Asks for rig.get_vfo and
// rig.get_mode only. While flrig is not answering it retries every 15 s.
class FlrigClient : public QObject {
    Q_OBJECT
public:
    explicit FlrigClient(QString host = "127.0.0.1", quint16 port = 12345, QObject* parent = nullptr);
    void start();
    void stop();
signals:
    void reading(double hz, const QString& mode);   // mode may be empty
    void lost();
private:
    void poll();
    void fail();
    QString host_;
    quint16 port_;
    QNetworkAccessManager net_;
    QTimer timer_;
    bool busy_ = false, up_ = false;
};

} // namespace hamclock
