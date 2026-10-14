#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include <QTcpSocket>
#include <QTimer>
#include <optional>

namespace hamclock {

// Read-only rigctld client (Hamlib, localhost by default). Polls the frequency
// and mode with the read-only "f" and "m" commands only; nothing that could
// change the rig or key the transmitter is ever sent. Without rigctld it
// reports "unconfigured".
class RadioModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasRig READ hasRig NOTIFY changed)
    Q_PROPERTY(QString frequency READ frequency NOTIFY changed)
    Q_PROPERTY(QString band READ band NOTIFY changed)
    Q_PROPERTY(QString mode READ mode NOTIFY changed)
public:
    explicit RadioModel(const WidgetContext& context, QString host = "127.0.0.1", quint16 port = 4532, QObject* parent = nullptr);
    void start();
    bool hasRig() const { return hz_.has_value(); }
    QString frequency() const;
    QString band() const;
    QString mode() const { return mode_; }
signals:
    void changed();
private slots:
    void connectNow();
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void poll();
private:
    enum class Wait { Idle, Freq, ModeName, ModePassband };
    void sendNext(const char* cmd, Wait w);
    void handleLine(const QString& line);
    void fail(const QString& text);
    QString host_;
    quint16 port_;
    QTcpSocket socket_;
    QTimer retry_, poll_;
    QByteArray buffer_;
    Wait wait_ = Wait::Idle;
    std::optional<double> hz_;
    QString mode_;
};

void register_widget_radio(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
