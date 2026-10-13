#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "quakes_parse.hpp"
#include <QVariantList>

namespace hamclock {

// Worldwide earthquakes M2.5+ in the past day (USGS). Shows the strongest
// event and the most recent few; values are "—" until a feed has been parsed.
class QuakesModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY quakesChanged)
    Q_PROPERTY(int count READ count NOTIFY quakesChanged)
    Q_PROPERTY(double strongestMagnitude READ strongestMagnitude NOTIFY quakesChanged)
    Q_PROPERTY(QString strongestText READ strongestText NOTIFY quakesChanged)
    Q_PROPERTY(QString strongestPlace READ strongestPlace NOTIFY quakesChanged)
    Q_PROPERTY(QVariantList recent READ recent NOTIFY quakesChanged)
public:
    explicit QuakesModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return feed_.ok; }
    int count() const { return static_cast<int>(feed_.quakes.size()); }
    double strongestMagnitude() const;
    QString strongestText() const;   // "Strong · 14 h ago"
    QString strongestPlace() const;
    QVariantList recent() const;     // newest first: {mag, place, age}

    PolledSource& source() { return *source_; }

signals:
    void quakesChanged();

private:
    void refreshState();

    PolledSource* source_;
    quakes::Feed feed_;
};

void register_widget_quakes(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
