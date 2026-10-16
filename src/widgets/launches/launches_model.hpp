#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "launches_parse.hpp"
#include "../../map/overlay.hpp"
#include <QVariantList>

namespace hamclock {

// Upcoming orbital launches (Launch Library 2, The Space Devs). Values are
// empty until a list has been parsed; past launches are never shown as upcoming.
class LaunchesModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY launchesChanged)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY launchesChanged)
    Q_PROPERTY(QString nextName READ nextName NOTIFY launchesChanged)
    Q_PROPERTY(QString nextCountdown READ nextCountdown NOTIFY launchesChanged)
    Q_PROPERTY(QString nextDetail READ nextDetail NOTIFY launchesChanged)
    Q_PROPERTY(QVariantList later READ later NOTIFY launchesChanged)
public:
    explicit LaunchesModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return feed_.ok; }
    bool hasNext() const;
    QString nextName() const;
    QString nextCountdown() const;
    QString nextDetail() const;   // "SpaceX · Vandenberg SFB, CA, USA · Go"
    QVariantList later() const;   // the next launches after the first: {name, when, status}
    PolledSource& source() { return *source_; }

signals:
    void launchesChanged();

private:
    void refreshState();
    void updateMapLayer();

    PolledSource* source_;
    launches::Feed feed_;
};

void register_widget_launches(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
