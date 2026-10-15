#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "psk_parse.hpp"
#include "../../net/polled_source.hpp"
#include <QVariantList>

namespace hamclock {

// Who heard my callsign in the last 15 minutes (PSKReporter). Unconfigured
// until a callsign is set in Settings.
class PskModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY reportsChanged)
    Q_PROPERTY(int count READ count NOTIFY reportsChanged)
    Q_PROPERTY(QString bestSnr READ bestSnr NOTIFY reportsChanged)
    Q_PROPERTY(QString bands READ bands NOTIFY reportsChanged)
    Q_PROPERTY(QVariantList recent READ recent NOTIFY reportsChanged)
public:
    explicit PskModel(const WidgetContext& context, QObject* parent = nullptr);
    ~PskModel() override;
    void setMapVisible(bool visible);  // per-source map on/off (default on)
    void start();  // (re)reads the callsign from settings

    bool hasData() const { return feed_.ok; }
    int count() const { return static_cast<int>(feed_.reports.size()); }
    QString bestSnr() const;
    QString bands() const;  // "20m 5 · 40m 2": reports per band, busiest first
    QVariantList recent() const;  // newest first: {call, grid, band, mode, snr, age}
    PolledSource* source() { return source_; }

signals:
    void reportsChanged();

private:
    void refreshState();
    void refreshStateImpl();
    void publishOverlay();

    AppSettings* settings_;
    Transport transport_;
    PolledSource* source_ = nullptr;
    QString call_;
    psk::Feed feed_;
    bool map_visible_ = true;
};

void register_widget_pskreporter(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
