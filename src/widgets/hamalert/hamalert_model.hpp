#pragma once
// HamAlert.org: personal spot alerts delivered over its telnet feed (hamalert.org:7300), in DX cluster
// "DX de" format. Needs a HamAlert user name and password (Settings; password via AppSettings, C++ only).
// Receive-only: the client never sends anything but the login and password.
#include "../../net/telnet_client.hpp"
#include "../dxcluster/dx_spot_parse.hpp"
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../dxcluster/dx_overlay.hpp"
#include <QVariantList>
#include <deque>

namespace hamclock {

class HamAlertModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QVariantList recent READ recent NOTIFY changed)
public:
    HamAlertModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent = nullptr);
    void start(); // (re)reads the login from settings; "Not configured" while user or password is empty
    int count() const { return int(spots_.size()); }
    QVariantList recent() const; // newest first, at most kShown rows
    ~HamAlertModel() override;
    void setMapVisible(bool visible); // per-source map on/off (default on)
    void setEntityResolver(dxcluster::EntityResolver r) { resolver_ = std::move(r); publishOverlay(); } // tests
    static constexpr const char* kOverlayLayerId = "hamalert";
    TelnetClient& client() { return client_; }
    static constexpr const char* kHost = "hamalert.org";
    static constexpr int kPort = 7300;
    static constexpr int kCapacity = 30, kShown = 8;
signals:
    void changed();
private:
    void publishOverlay();
    bool map_visible_ = true;
    dxcluster::EntityResolver resolver_ = dxcluster::bundled_entity_resolver();
    void on_line(const QString& line);
    void on_state(TelnetClient::State state);
    AppSettings* settings_;
    TelnetClient client_;
    std::deque<dxcluster::DxSpot> spots_;
};

void register_widget_hamalert(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
