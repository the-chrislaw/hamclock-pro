#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include <QNetworkAccessManager>
#include <QTimer>
#include <optional>

namespace hamclock {

// Read-only PiAware / dump1090 client: polls the receiver's own aircraft.json
// (localhost by default) and shows how many aircraft it hears. Without a
// receiver it reports "unconfigured". No map overlay yet.
class AdsbModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasReceiver READ hasReceiver NOTIFY changed)
    Q_PROPERTY(QString tracked READ tracked NOTIFY changed)
    Q_PROPERTY(QString withPosition READ withPosition NOTIFY changed)
public:
    explicit AdsbModel(const WidgetContext& context, QString url = "http://127.0.0.1:8080/data/aircraft.json", QObject* parent = nullptr);
    void start();
    bool hasReceiver() const { return tracked_.has_value(); }
    QString tracked() const { return tracked_ ? QString::number(*tracked_) : QString(); }
    QString withPosition() const { return tracked_ ? QString::number(positioned_) : QString(); }
signals:
    void changed();
private slots:
    void poll();
private:
    void fail(const QString& state, const QString& text);
    QString url_;
    QNetworkAccessManager net_;
    QTimer timer_;
    bool busy_ = false, started_ = false;
    std::optional<int> tracked_;
    int positioned_ = 0;
};

void register_widget_adsb(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
