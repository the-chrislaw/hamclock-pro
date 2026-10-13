#pragma once
// Settings exposed to QML. Station fields live in the existing station file
// (platform/settings, shared with the CLI and the legacy editor); app options
// in an INI file; account credentials in a separate owner-only (0600) file so
// they never sit next to shareable settings. All files are under the per-user
// config directory unless an explicit station path is given.
#include "../platform/settings.hpp"
#include <QObject>
#include <QString>
#include <QStringList>

namespace hamclock {

class AppSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString callsign READ callsign NOTIFY stationChanged)
    Q_PROPERTY(double deLatitude READ deLatitude NOTIFY stationChanged)
    Q_PROPERTY(double deLongitude READ deLongitude NOTIFY stationChanged)
    Q_PROPERTY(double dxLatitude READ dxLatitude NOTIFY stationChanged)
    Q_PROPERTY(double dxLongitude READ dxLongitude NOTIFY stationChanged)
    Q_PROPERTY(QString deZone READ deZone NOTIFY stationChanged)
    Q_PROPERTY(QString appearance READ appearance WRITE setAppearance NOTIFY appearanceChanged)
    Q_PROPERTY(bool configured READ configured NOTIFY stationChanged)
    Q_PROPERTY(QString clusterHost READ clusterHost NOTIFY optionsChanged)
    Q_PROPERTY(int clusterPort READ clusterPort NOTIFY optionsChanged)
    Q_PROPERTY(QString lookupService READ lookupService NOTIFY accountsChanged)
    Q_PROPERTY(QString lookupUser READ lookupUser NOTIFY accountsChanged)
    // Widget id shown in each pane slot of the dashboard ("none" = empty slot).
    Q_PROPERTY(QStringList panes READ panes NOTIFY panesChanged)
    Q_PROPERTY(bool hasLookupPassword READ hasLookupPassword NOTIFY accountsChanged)
public:
    // station_path empty = <config>/station.conf. config_dir empty = per-user default.
    explicit AppSettings(QString station_path = {}, QString config_dir = {}, QObject* parent = nullptr);

    QString callsign() const { return QString::fromStdString(station_.callsign); }
    double deLatitude() const { return station_.de.latitude; }
    double deLongitude() const { return station_.de.longitude; }
    double dxLatitude() const { return station_.dx.latitude; }
    double dxLongitude() const { return station_.dx.longitude; }
    QString deZone() const { return QString::fromStdString(station_.de_zone); }
    QString appearance() const { return QString::fromStdString(station_.appearance); }
    bool configured() const { return configured_; }
    const StationSettings& station() const { return station_; }
    QString clusterHost() const { return cluster_host_; }
    int clusterPort() const { return cluster_port_; }
    QString lookupService() const { return lookup_service_; }
    QString lookupUser() const { return lookup_user_; }
    bool hasLookupPassword() const { return !lookup_password_.isEmpty(); }
    // C++ only (never a QML property): for widgets that log in.
    QString lookupPassword() const { return lookup_password_; }
    QString stationPath() const { return station_path_; }
    QString loadError() const { return load_error_; }

    static constexpr int kPaneSlots = 3;
    QStringList panes() const;
    // Returns an empty string or a user-facing error.
    Q_INVOKABLE QString setPane(int slot, const QString& widget_id);
    Q_INVOKABLE void resetPanes();

    void setAppearance(const QString& appearance);
    // Validate and save; returns an empty string or a user-facing error.
    Q_INVOKABLE QString saveStation(const QString& callsign, double de_lat, double de_lon,
                                    double dx_lat, double dx_lon, const QString& de_zone);
    Q_INVOKABLE QString saveCluster(const QString& host, int port);
    // Empty password keeps the stored one; service: "none", "qrz" or "hamqth".
    Q_INVOKABLE QString saveAccount(const QString& service, const QString& user, const QString& password);
    Q_INVOKABLE void clearAccount();

signals:
    void stationChanged();
    void appearanceChanged();
    void optionsChanged();
    void accountsChanged();
    void panesChanged();

private:
    void loadOptions();
    void loadAccounts();
    QString writeAccounts();

    QString config_dir_, station_path_, load_error_;
    StationSettings station_;
    bool configured_ = false;
    QString cluster_host_;
    int cluster_port_ = 7300;
    QStringList panes_;   // empty entry = default for that slot
    QString lookup_service_ = "none", lookup_user_, lookup_password_;
};

} // namespace hamclock
