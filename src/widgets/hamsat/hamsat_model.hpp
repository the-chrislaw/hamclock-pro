#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "hamsat_parse.hpp"
#include <QVariantList>

namespace hamclock { class AppSettings; }

namespace hamclock {

// Upcoming amateur-satellite activations announced on hams.at. Values are
// empty until a list has been parsed; finished passes are never shown.
class HamsatModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY activationsChanged)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY activationsChanged)
    Q_PROPERTY(QString nextWhen READ nextWhen NOTIFY activationsChanged)
    Q_PROPERTY(QString nextTitle READ nextTitle NOTIFY activationsChanged)
    Q_PROPERTY(QString nextDetail READ nextDetail NOTIFY activationsChanged)
    Q_PROPERTY(QVariantList later READ later NOTIFY activationsChanged)
    Q_PROPERTY(bool personalised READ personalised NOTIFY activationsChanged)
public:
    explicit HamsatModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return feed_.ok; }
    bool hasNext() const;
    QString nextWhen() const;    // "in 3h 12m"
    QString nextTitle() const;   // "KB5FHK on RS-44"
    QString nextDetail() const;  // "SSB · 436.795 MHz · FN00oi · 01:04Z-01:26Z"
    QVariantList later() const;  // {when, title}
    // True while the last list was fetched with the user's API key and so carries per-station values.
    bool personalised() const { return feed_.ok && fetched_with_key_; }
    PolledSource& source() { return *source_; }

signals:
    void activationsChanged();

private:
    void refreshState();
    QString station_text(const hamsat::Activation& a) const;

    PolledSource* source_ = nullptr;
    hamsat::Feed feed_;
    AppSettings* settings_;
    bool fetched_with_key_ = false;
    bool attempt_with_key_ = false;
    bool key_rejected_ = false;
};

void register_widget_hamsat(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
