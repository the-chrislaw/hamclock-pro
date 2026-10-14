#pragma once
// Nearby APRS stations from APRS-IS (receive-only: logs in with passcode -1, server-side range filter around DE,
// never sends a packet). Needs a callsign in Settings; "Not configured" while it is empty.
#include "../../net/telnet_client.hpp"
#include "aprs_parse.hpp"
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include <QHash>
#include <QVariantList>

namespace hamclock {

class AprsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QVariantList nearest READ nearest NOTIFY changed)
public:
    AprsModel(const WidgetContext& context, TelnetSocketFactory factory, QObject* parent = nullptr);
    void start();
    int count() const { return int(stations_.size()); }
    QVariantList nearest() const; // closest to DE first, at most kShown rows
    static QString login_line(const QString& call, double lat, double lon, int radius_km);
    static constexpr const char* kHost = "rotate.aprs2.net";
    static constexpr int kPort = 14580, kRadiusKm = 100, kCapacity = 200, kShown = 6, kMaxAgeSec = 1800;
signals:
    void changed();
private:
    struct Entry { aprs::Station station; QDateTime heard; };
    void connect_now();
    void on_data(const QByteArray& data);
    void on_closed();
    void prune();
    AppSettings* settings_;
    TelnetSocketFactory factory_;
    TelnetSocket* socket_ = nullptr;
    QTimer retry_;
    QString login_;
    QByteArray partial_;
    QHash<QString, Entry> stations_;
    int failures_ = 0;
    bool wanted_ = false;
};

void register_widget_aprs(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
