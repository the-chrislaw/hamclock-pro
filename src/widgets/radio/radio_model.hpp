#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "flrig_client.hpp"
#include "radio_parse.hpp"
#include <QTcpSocket>
#include <QTimer>
#include <optional>

namespace hamclock {

// Read-only rigctld client (Hamlib, localhost by default); falls back to a
// read-only flrig XML-RPC poller when rigctld is not running. Polls the frequency
// and mode with the read-only "f" and "m" commands only; nothing that could
// change the rig or key the transmitter is ever sent. Without rigctld it
// reports "unconfigured".
class RadioModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasRig READ hasRig NOTIFY changed)
    Q_PROPERTY(QString frequency READ frequency NOTIFY changed)
    Q_PROPERTY(QString band READ band NOTIFY changed)
    Q_PROPERTY(QString mode READ mode NOTIFY changed)
    Q_PROPERTY(QString passband READ passband NOTIFY changed)
public:
    explicit RadioModel(const WidgetContext& context, QString host = "127.0.0.1", quint16 port = 4532, QObject* parent = nullptr);
    void start();
    // Settings changed the rigctld address: drop the current link and connect to the new one.
    void setEndpoint(const QString& host, quint16 port);
    bool hasRig() const { return hz_.has_value(); }
    QString frequency() const;
    QString band() const;
    QString mode() const { return mode_; }
    QString passband() const { return passband_ ? radio::format_passband(*passband_) : QString(); }
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
    enum class Wait { Idle, Freq, ModeName, ModePassband };
    void sendNext(const char* cmd, Wait w);
    void handleLine(const QString& line);
    void fail(const QString& text);
    QString unreachableText() const;
    QString host_;
    quint16 port_;
    QTcpSocket socket_;
    bool started_ = false;
    QTimer retry_, poll_;
    FlrigClient flrig_;
    bool fromFlrig_ = false;
    QByteArray buffer_;
    Wait wait_ = Wait::Idle;
    std::optional<double> hz_;
    QString mode_;
    std::optional<int> passband_;
};

void register_widget_radio(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
