#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../../domain/astro_finder.hpp"
#include <QTimer>
#include <QStringList>
#include <QVariantList>
#include <set>

namespace hamclock {

class AppSettings;

// "Passes over you": the next 48 h of passes of every satellite with fresh elements at DE, from
// the same CelesTrak elements as the Satellites pane (fed by it), propagated offline with SGP4.
class PassesModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY changed)
    // {name, aos (unix s), day, utc, local, maxEl, from, to, duration, visible, live} soonest first.
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(QString filter READ filter NOTIFY changed)   // "all", "amateur", "visible" or "starred"
    Q_PROPERTY(QString emptyReason READ emptyReason NOTIFY changed)
public:
    explicit PassesModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();
    // Elements to schedule (stale sets are dropped here) and the NORAD ids that have SatNOGS
    // transmitters (the Amateur filter).
    void setElements(const std::vector<Tle>& tles, const std::set<int>& amateur_norads);
    void recompute();

    bool hasData() const { return !rows_.isEmpty(); }
    QVariantList rows() const { return rows_; }
    QString filter() const { return filter_; }
    QString emptyReason() const { return empty_reason_; }
    Q_INVOKABLE void setFilter(const QString& filter);
    // Star or unstar a satellite by its short name; saved to `[passes] starred=` (comma separated).
    Q_INVOKABLE void toggleStar(const QString& name);
    bool isStarred(const QString& name) const { return starred_.contains(name); }

signals:
    void changed();

private:
    AppSettings* settings_;
    std::vector<Tle> tles_;
    std::set<int> amateur_;
    QString filter_ = "all";
    QStringList starred_;
    QVariantList rows_;
    QString empty_reason_;
    QTimer timer_;
};

// "Sat 14:05" style day label for a UTC unix time relative to `now` ("Today"/"Tomorrow"/"Sun").
QString pass_day_label(qint64 unix_time, qint64 now);

void register_widget_passes(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
