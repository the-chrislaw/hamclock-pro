#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "firewx_parse.hpp"
#include <QVariantList>

namespace hamclock {

// Active NWS fire-weather warnings and watches (US). Alerts past their
// expiry are dropped using the model clock, so an old cache never shows as current.
class FirewxModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY alertsChanged)
    Q_PROPERTY(int count READ count NOTIFY alertsChanged)
    Q_PROPERTY(int topRank READ topRank NOTIFY alertsChanged)
    Q_PROPERTY(QString headline READ headline NOTIFY alertsChanged)
    Q_PROPERTY(QString shortHeadline READ shortHeadline NOTIFY alertsChanged)
    Q_PROPERTY(QString headlineArea READ headlineArea NOTIFY alertsChanged)
    Q_PROPERTY(QString headlineTimes READ headlineTimes NOTIFY alertsChanged)
    Q_PROPERTY(QString headlineStart READ headlineStart NOTIFY alertsChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY alertsChanged)
    Q_PROPERTY(QString atDe READ atDe NOTIFY alertsChanged)
public:
    explicit FirewxModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return feed_.ok; }
    int count() const { return static_cast<int>(active().size()); }
    int topRank() const;
    QString headline() const;       // "Red Flag Warning"
    QString shortHeadline() const;  // "Red Flag": fits a narrow card
    QString headlineArea() const;   // area text of the worst alert
    QString headlineTimes() const;  // "issued 13 20:42Z · until 14 04:00Z"
    QString headlineStart() const;  // "starts 15 13:00Z" while the alert is still ahead
    QString summary() const;        // "2 Red Flag Warning · 1 Fire Weather Watch"

    // "At DE: Red Flag Warning · until 17 05:00Z" or "At DE: none active"; empty
    // without a configured station or before the point query has answered.
    QString atDe() const;

    PolledSource& source() { return *source_; }
    PolledSource* pointSource() { return point_; }

signals:
    void alertsChanged();

private:
    std::vector<firewx::Alert> active() const;
    void refreshState();
    void resetPointSource();

    WidgetContext context_;
    PolledSource* source_;
    PolledSource* point_ = nullptr;
    firewx::Feed feed_;
    firewx::Feed point_feed_;
};

void register_widget_firewx(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
