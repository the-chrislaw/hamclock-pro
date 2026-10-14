#pragma once
#include "../../net/telnet_client.hpp"
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "dx_spot_parse.hpp"
#include "udp_spot.hpp"
#include <QAbstractListModel>
#include <QVariantList>
#include <functional>
#include <deque>
#include <vector>

namespace hamclock {

// Recent spots, newest first.
class DxSpotList : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { DxRole = Qt::UserRole + 1, SpotterRole, FrequencyRole, BandRole, ModeRole, TimeRole, CommentRole, WatchedRole, CountryRole };
    using QAbstractListModel::QAbstractListModel;
    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : int(shown_.size()); }
    int totalCount() const { return int(spots_.size()); }
    void set_filter(const QString& band, const QString& mode); // empty = any
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void add(const dxcluster::DxSpot& spot);
    const std::deque<dxcluster::DxSpot>& spots() const { return spots_; }
    void set_watch(const QStringList& entries); // re-evaluates every stored spot
    int watchedCount() const { return watched_; }  // spots in the list (any filter) on the watch list
    // Per-band spot counts by continent over every stored spot (any filter): [{band, total, NA, SA, EU, AF, AS, OC}].
    // Default resolver is the bundled DXCC table.
    QVariantList activity(const std::function<QString(const QString&)>& continent_of = {}) const;
    static constexpr int kCapacity = 50;
private:
    bool matches(const dxcluster::DxSpot& s) const {
        return (band_.isEmpty() || s.band == band_) && (mode_.isEmpty() || s.mode == mode_);
    }
    std::deque<dxcluster::DxSpot> spots_; // every spot, newest first
    std::vector<int> shown_;              // indexes into spots_ passing the filter
    void rebuild();
    QString band_, mode_;
    QStringList watch_;
    std::vector<char> is_watched_; // parallel to spots_
    int watched_ = 0;
};

class DxClusterModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(QObject* spots READ spots CONSTANT)
    Q_PROPERTY(int spotCount READ spotCount NOTIFY countChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countChanged)
    Q_PROPERTY(int watchedCount READ watchedCount NOTIFY countChanged)
    Q_PROPERTY(QVariantList activity READ activity NOTIFY countChanged)
    Q_PROPERTY(QString bandFilter READ bandFilter WRITE setBandFilter NOTIFY filterChanged)
    Q_PROPERTY(QString modeFilter READ modeFilter WRITE setModeFilter NOTIFY filterChanged)
public:
    DxClusterModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent = nullptr);
    void start(); // (re)reads host/port/callsign from settings; unconfigured when any is empty
    QObject* spots() { return &list_; }
    int spotCount() const { return list_.rowCount(); }
    int totalCount() const { return list_.totalCount(); }
    int watchedCount() const { return list_.watchedCount(); }
    QVariantList activity() const { return list_.activity(); }
    void refreshWatchList(); // re-reads the watch list from settings
    void setMapVisible(bool visible); // per-source map on/off (default on)
    void refreshUdp();       // (re)binds the UDP spot input from settings; off when the port is 0
    dxcluster::UdpSpotListener& udp() { return udp_; }
    QString bandFilter() const { return band_filter_; }
    QString modeFilter() const { return mode_filter_; }
    // Unknown values are ignored; "" shows everything.
    Q_INVOKABLE void setBandFilter(const QString& band);
    Q_INVOKABLE void setModeFilter(const QString& mode);
    Q_INVOKABLE void cycleBandFilter();
    Q_INVOKABLE void cycleModeFilter();
    // Callsign plus optional -SSID (1-99); empty when the callsign is empty.
    static QString cluster_login(const QString& callsign, int ssid);
    static QStringList bandChoices();
    static QStringList modeChoices();
    TelnetClient& client() { return client_; }
    ~DxClusterModel() override;
signals:
    void countChanged();
    void filterChanged();
private:
    void publishOverlay();
    bool map_visible_ = true;
    void on_line(const QString& line);
    void on_state(TelnetClient::State state);
    AppSettings* settings_;
    TelnetClient client_;
    DxSpotList list_;
    dxcluster::UdpSpotListener udp_;
    QString band_filter_, mode_filter_;
};

void register_widget_dxcluster(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
