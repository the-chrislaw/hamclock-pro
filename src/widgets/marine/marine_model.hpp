#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "marine_parse.hpp"
#include <QVariantList>

namespace hamclock {

// Active NWS marine warnings and advisories (US waters). Alerts past their
// expiry are dropped using the model clock, so an old cache never shows as current.
class MarineModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY alertsChanged)
    Q_PROPERTY(int count READ count NOTIFY alertsChanged)
    Q_PROPERTY(int topRank READ topRank NOTIFY alertsChanged)
    Q_PROPERTY(QString headline READ headline NOTIFY alertsChanged)
    Q_PROPERTY(QString headlineArea READ headlineArea NOTIFY alertsChanged)
    Q_PROPERTY(QString headlineTimes READ headlineTimes NOTIFY alertsChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY alertsChanged)
public:
    explicit MarineModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return feed_.ok; }
    int count() const { return static_cast<int>(active().size()); }
    int topRank() const;
    QString headline() const;       // "Gale Warning"
    QString headlineArea() const;   // area text of the worst alert
    QString headlineTimes() const;  // "issued 13 20:42Z · until 14 04:00Z"
    QString summary() const;        // "26 Gale Warning · 76 Small Craft Advisory"

    PolledSource& source() { return *source_; }

signals:
    void alertsChanged();

private:
    std::vector<marine::Alert> active() const;
    void refreshState();

    PolledSource* source_;
    marine::Feed feed_;
};

void register_widget_marine(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
