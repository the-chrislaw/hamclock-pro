#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "ota_parse.hpp"
#include <QVariantList>

namespace hamclock {

// Parks and Summits On The Air activator spots (public POTA and SOTA APIs).
// Either feed alone is enough to show data; the other is reported in the status.
class OtaModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY spotsChanged)
    Q_PROPERTY(int count READ count NOTIFY spotsChanged)
    Q_PROPERTY(QVariantList recent READ recent NOTIFY spotsChanged)
public:
    explicit OtaModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return pota_feed_.ok || sota_feed_.ok; }
    int count() const { return static_cast<int>(spots_.size()); }
    QVariantList recent() const;  // newest first: {call, ref, freq, mode, program, age}

    // Spots are drawn on the map only while the data is live; off hides them.
    void setMapVisible(bool visible);
    bool mapVisible() const { return map_visible_; }
    // Display filter for the list, the count and the map layer.
    void setFilter(const ota::Filter& filter);
    const ota::Filter& filter() const { return filter_; }
    ~OtaModel() override;

    PolledSource& potaSource() { return *pota_; }
    PolledSource& sotaSource() { return *sota_; }

signals:
    void spotsChanged();

private:
    void reparse();
    void applyFilter();
    void refreshState();
    void refreshStateImpl();
    void publishOverlay();

    PolledSource* pota_;
    PolledSource* sota_;
    ota::Feed pota_feed_, sota_feed_;
    std::vector<ota::Spot> all_spots_;
    std::vector<ota::Spot> spots_;  // after the display filter
    ota::Filter filter_;
    bool map_visible_ = true;
};

void register_widget_ota(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
