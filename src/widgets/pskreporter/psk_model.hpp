#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "psk_parse.hpp"
#include "../../net/polled_source.hpp"
#include "../../net/telnet_client.hpp"
#include <QVariantList>
#include <memory>

namespace hamclock {

// Who heard my callsign in the last 15 minutes (PSKReporter). Unconfigured
// until a callsign is set in Settings.
class PskModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY reportsChanged)
    Q_PROPERTY(int count READ count NOTIFY reportsChanged)
    Q_PROPERTY(QString bestSnr READ bestSnr NOTIFY reportsChanged)
    Q_PROPERTY(QString bands READ bands NOTIFY reportsChanged)
    Q_PROPERTY(QString modes READ modes NOTIFY reportsChanged)
    Q_PROPERTY(QString farthest READ farthest NOTIFY reportsChanged)
    Q_PROPERTY(QString heard READ heard NOTIFY reportsChanged)
    Q_PROPERTY(QVariantList recent READ recent NOTIFY reportsChanged)
public:
    // `socket_factory` enables the opt-in RBN telnet feed; without one RBN is never contacted.
    explicit PskModel(const WidgetContext& context, TelnetSocketFactory socket_factory = {}, QObject* parent = nullptr);
    ~PskModel() override;
    void setMapVisible(bool visible);  // per-source map on/off (default on)
    void start();  // (re)reads the callsign from settings

    bool hasData() const { return feed_.ok; }
    int count() const { return static_cast<int>(feed_.reports.size()); }
    QString bestSnr() const;
    QString bands() const;  // "20m 5 · 40m 2": reports per band, busiest first
    QString modes() const;  // "FT8 6 · CW 2": reports per mode, busiest first
    QString farthest() const;  // "Farthest: JA1XYZ 9,876 km 20m" (miles if imperial), empty if none
    QString heard() const;  // WSPR heard by DE: "I heard 12 · farthest JA1XYZ 9,876 km 20m" (opt-in), empty if none
    QVariantList recent() const;  // newest first: {call, grid, band, mode, snr, age}
    PolledSource* source() { return source_; }
    TelnetClient* rbnClient() { return rbn_.get(); }

signals:
    void reportsChanged();

private:
    void applyFilter();
    void syncWspr();
    void syncRbn();
    void onRbnLine(const QString& line);
    void mergeRaw();
    void refreshState();
    void refreshStateImpl();
    void publishOverlay();

    AppSettings* settings_;
    Transport transport_;
    PolledSource* source_ = nullptr;
    QString call_;
    PolledSource* wspr_source_ = nullptr;
    PolledSource* heard_source_ = nullptr;
    TelnetSocketFactory socket_factory_;
    std::unique_ptr<TelnetClient> rbn_;  // one connection, receive-only, opt-in
    psk::Feed psk_raw_, wspr_raw_, heard_raw_, rbn_raw_;
    psk::Feed feed_, raw_;  // feed_ = raw_ limited to the max distance setting
    bool map_visible_ = true;
};

void register_widget_pskreporter(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
