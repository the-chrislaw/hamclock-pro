#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "fires_parse.hpp"

namespace hamclock {

class AppSettings;

// Satellite fire detections (NASA FIRMS, VIIRS NOAA-20, contiguous US and Hawaii,
// last 24 h). A successful fetch with no rows reads "No detections", not missing data.
class FiresModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY firesChanged)
    Q_PROPERTY(int count READ count NOTIFY firesChanged)
    Q_PROPERTY(int highCount READ highCount NOTIFY firesChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY firesChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY firesChanged)
    Q_PROPERTY(QString nearestText READ nearestText NOTIFY firesChanged)
public:
    explicit FiresModel(const WidgetContext& context, QObject* parent = nullptr);
    ~FiresModel() override;
    void start();

    bool hasData() const { return feed_.ok; }
    int count() const { return static_cast<int>(feed_.detections.size()); }
    int highCount() const;
    QString summary() const;  // "1,300 detections"
    QString detail() const;   // "strongest 120 MW at 32.1N 86.8W · newest 13 18:38Z"

    // "Nearest 320 km NW of DE · 13 18:38Z" (non-low confidence); empty without a saved station.
    QString nearestText() const;

    // Detections are drawn on the map only while the data is live.
    void setMapVisible(bool visible);
    bool mapVisible() const { return map_visible_; }

    PolledSource& source() { return *source_; }

signals:
    void firesChanged();

private:
    void refreshState();
    void refreshStateImpl();
    void publishOverlay();

    AppSettings* settings_;
    PolledSource* source_;
    fires::Feed feed_;
    bool map_visible_ = true;
};

void register_widget_fires(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
