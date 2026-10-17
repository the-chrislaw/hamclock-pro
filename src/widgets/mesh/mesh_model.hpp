#pragma once
// Meshtastic nodes with positions, read from the user's own node over its LAN TCP API (receive-only; the only
// write is the "send me your node list" request). Opt-in: "Not configured" until a host is set in Settings.
#include "../../net/telnet_client.hpp"
#include "mesh_parse.hpp"
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include <QHash>
#include <QVariantList>

namespace hamclock {

class MeshModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QVariantList nearest READ nearest NOTIFY changed)
public:
    MeshModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent = nullptr);
    ~MeshModel() override;
    void start();
    void setMapVisible(bool visible);
    int count() const { return int(nodes_.size()); }
    QVariantList nearest() const; // closest to DE first, at most kShown rows
    static constexpr int kCapacity = 300, kShown = 6;
    static constexpr qint64 kMaxAgeSec = 2 * 86400;
signals:
    void changed();
private:
    void connect_now();
    void on_data(const QByteArray& data);
    void on_closed();
    void publishOverlay();
    AppSettings* settings_;
    TelnetSocketFactory factory_;
    TelnetSocket* socket_ = nullptr;
    QTimer retry_;
    QByteArray partial_;
    QHash<quint32, mesh::Node> nodes_;
    int failures_ = 0;
    bool wanted_ = false;
    bool map_visible_ = true;
};

void register_widget_mesh(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
