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

    // Quakes are drawn on the map only while the data is live; off hides them.
    void setMapVisible(bool visible);
    bool mapVisible() const { return map_visible_; }
    ~QuakesModel() override;

    PolledSource& source() { return *source_; }

signals:
    void quakesChanged();

private:
    void refreshState();
    void refreshStateImpl();
    void publishOverlay();

    PolledSource* source_;
    quakes::Feed feed_;
    bool map_visible_ = true;
};

void register_widget_quakes(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
