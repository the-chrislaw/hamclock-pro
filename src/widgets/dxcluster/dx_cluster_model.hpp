#pragma once
#include "../../net/telnet_client.hpp"
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "dx_spot_parse.hpp"
#include <QAbstractListModel>
#include <deque>

namespace hamclock {

// Recent spots, newest first.
class DxSpotList : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { DxRole = Qt::UserRole + 1, SpotterRole, FrequencyRole, BandRole, ModeRole, TimeRole, CommentRole };
    using QAbstractListModel::QAbstractListModel;
    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : int(spots_.size()); }
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void add(const dxcluster::DxSpot& spot);
    static constexpr int kCapacity = 50;
private:
    std::deque<dxcluster::DxSpot> spots_;
};

class DxClusterModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(QObject* spots READ spots CONSTANT)
    Q_PROPERTY(int spotCount READ spotCount NOTIFY countChanged)
public:
    DxClusterModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent = nullptr);
    void start(); // (re)reads host/port/callsign from settings; unconfigured when any is empty
    QObject* spots() { return &list_; }
    int spotCount() const { return list_.rowCount(); }
    TelnetClient& client() { return client_; }
signals:
    void countChanged();
private:
    void on_line(const QString& line);
    void on_state(TelnetClient::State state);
    AppSettings* settings_;
    TelnetClient client_;
    DxSpotList list_;
};

void register_widget_dxcluster(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
