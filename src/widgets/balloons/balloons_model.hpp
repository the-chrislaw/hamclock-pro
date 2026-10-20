#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "balloons_parse.hpp"
#include "../../map/overlay.hpp"
#include <QVariantList>

namespace hamclock {

class AppSettings;

// Radiosondes (weather balloons) heard recently, from SondeHub. Values are
// empty until telemetry has been parsed; sondes not heard for 30 minutes are
// never shown as flying.
class BalloonsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY balloonsChanged)
    Q_PROPERTY(bool hasStation READ hasStation NOTIFY balloonsChanged)
    Q_PROPERTY(int count READ count NOTIFY balloonsChanged)
    Q_PROPERTY(QString nearest READ nearest NOTIFY balloonsChanged)
    Q_PROPERTY(QString nearestDetail READ nearestDetail NOTIFY balloonsChanged)
    Q_PROPERTY(QVariantList later READ later NOTIFY balloonsChanged)
public:
    explicit BalloonsModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return feed_.ok; }
    bool hasStation() const;
    int count() const;
    QString nearest() const;        // "312 km NW"
    QString nearestDetail() const;  // "RS41 V1710894 · 18.5 km · climbing · 401.000 MHz"
    QVariantList later() const;     // next nearest: {dist, text}
    PolledSource& source() { return *source_; }

signals:
    void balloonsChanged();

private:
    void refreshState();
    void updateMapLayer();
    std::vector<balloons::Near> near(int limit) const;

    AppSettings* settings_;
    PolledSource* source_;
    balloons::Feed feed_;
};

void register_widget_balloons(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
