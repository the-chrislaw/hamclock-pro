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
    Q_PROPERTY(int clusterSsid READ clusterSsid NOTIFY optionsChanged)
    Q_PROPERTY(QString clusterCommandsText READ clusterCommandsText NOTIFY optionsChanged)
    Q_PROPERTY(QString watchListText READ watchListText NOTIFY optionsChanged)
    Q_PROPERTY(QString adifPath READ adifPath NOTIFY optionsChanged)
    Q_PROPERTY(QString otaProgram READ otaProgram NOTIFY optionsChanged)
    Q_PROPERTY(QString otaBand READ otaBand NOTIFY optionsChanged)
    Q_PROPERTY(QString otaMode READ otaMode NOTIFY optionsChanged)
    Q_PROPERTY(int otaMaxAgeMin READ otaMaxAgeMin NOTIFY optionsChanged)
    Q_PROPERTY(QString otaSort READ otaSort NOTIFY optionsChanged)
    Q_PROPERTY(QString aprsHost READ aprsHost NOTIFY optionsChanged)
    Q_PROPERTY(int aprsPort READ aprsPort NOTIFY optionsChanged)
    Q_PROPERTY(int pskMaxKm READ pskMaxKm NOTIFY optionsChanged)
    Q_PROPERTY(QString pskMatch READ pskMatch NOTIFY optionsChanged)
    Q_PROPERTY(int aprsRadiusKm READ aprsRadiusKm NOTIFY optionsChanged)
    Q_PROPERTY(int udpSpotPort READ udpSpotPort NOTIFY optionsChanged)
    Q_PROPERTY(bool imperialUnits READ imperialUnits NOTIFY optionsChanged)
    Q_PROPERTY(int dateOrder READ dateOrder NOTIFY optionsChanged)
    Q_PROPERTY(bool clock12h READ clock12h NOTIFY optionsChanged)
    Q_PROPERTY(int bannerStyle READ bannerStyle NOTIFY optionsChanged)
    Q_PROPERTY(int bannerBackground READ bannerBackground NOTIFY optionsChanged)
    Q_PROPERTY(QString bannerTitle READ bannerTitle NOTIFY optionsChanged)
    Q_PROPERTY(bool weekStartsSunday READ weekStartsSunday NOTIFY optionsChanged)
    Q_PROPERTY(int overscanPercent READ overscanPercent NOTIFY optionsChanged)
    Q_PROPERTY(bool udpSpotLan READ udpSpotLan NOTIFY optionsChanged)
    Q_PROPERTY(bool udpSpotByMe READ udpSpotByMe NOTIFY optionsChanged)
    Q_PROPERTY(QString lookupService READ lookupService NOTIFY accountsChanged)
    Q_PROPERTY(QString lookupUser READ lookupUser NOTIFY accountsChanged)
    // Widget id shown in each pane slot of the dashboard ("none" = empty slot).
    Q_PROPERTY(QStringList panes READ panes NOTIFY panesChanged)
    // Seconds between automatic widget changes per slot (0 = off; 5, 10, 30 or 60).
    Q_PROPERTY(QList<int> paneRotations READ paneRotations NOTIFY panesChanged)
    // Widgets a slot's rotation cycles through, one list per slot (empty list = every widget).
    Q_PROPERTY(QVariantList paneRotationSets READ paneRotationSets NOTIFY panesChanged)
    Q_PROPERTY(bool hasLookupPassword READ hasLookupPassword NOTIFY accountsChanged)
    Q_PROPERTY(QString hamAlertUser READ hamAlertUser NOTIFY accountsChanged)
    Q_PROPERTY(bool hasHamAlertPassword READ hasHamAlertPassword NOTIFY accountsChanged)
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
    int clusterSsid() const { return cluster_ssid_; }                  // 0 = none, else 1-99
    QStringList clusterCommands() const { return cluster_commands_; } // sent after login
    QString clusterCommandsText() const { return cluster_commands_.join('\n'); }
    QStringList watchList() const { return watch_list_; }              // uppercase calls / prefixes with *
    QString watchListText() const { return watch_list_.join(' '); }
    int udpSpotPort() const { return udp_port_; }                      // 0 = UDP spot input off
    int dateOrder() const { return date_order_; }                      // 0 D M Y, 1 M D Y, 2 ISO
    bool clock12h() const { return clock_12h_; }
    int bannerStyle() const { return banner_style_; }
    int bannerBackground() const { return banner_bg_; }
    QString bannerTitle() const { return banner_title_; }
    bool weekStartsSunday() const { return week_sunday_; }
    int overscanPercent() const { return overscan_; }
    bool imperialUnits() const { return imperial_; }                   // distances in miles
    bool udpSpotByMe() const { return udp_by_me_; }                    // true = only spots from my own call/programs
    bool udpSpotLan() const { return udp_lan_; }                       // false = 127.0.0.1 only
    // On-the-air display filter; empty / 0 = no restriction.
    QString otaProgram() const { return ota_program_; }  // "POTA", "SOTA" or empty
    QString otaBand() const { return ota_band_; }        // e.g. "20m"
    QString otaMode() const { return ota_mode_; }        // e.g. "CW"
    int otaMaxAgeMin() const { return ota_max_age_min_; }
    QString otaSort() const { return ota_sort_; }
    // APRS-IS server (receive-only) and search radius around DE.
    QString aprsHost() const { return aprs_host_; }
    int aprsPort() const { return aprs_port_; }
    int pskMaxKm() const { return psk_max_km_; }
    QString pskMatch() const { return psk_match_; }
    int aprsRadiusKm() const { return aprs_radius_km_; }
    QString adifPath() const { return adif_path_; }                    // local .adi log, empty = none
    QString lookupService() const { return lookup_service_; }
    QString lookupUser() const { return lookup_user_; }
    bool hasLookupPassword() const { return !lookup_password_.isEmpty(); }
    // C++ only (never a QML property): for widgets that log in.
    QString lookupPassword() const { return lookup_password_; }
    QString hamAlertUser() const { return hamalert_user_; }
    bool hasHamAlertPassword() const { return !hamalert_password_.isEmpty(); }
    QString hamAlertPassword() const { return hamalert_password_; } // C++ only, like lookupPassword()
    QString stationPath() const { return station_path_; }
    QString optionsPath() const { return config_dir_ + "/options.ini"; }
    QString loadError() const { return load_error_; }

    static constexpr int kPaneSlots = 3;
    static constexpr int kMaxRotationSet = 64;
    QStringList panes() const;
    // Returns an empty string or a user-facing error.
    Q_INVOKABLE QString setPane(int slot, const QString& widget_id);
    Q_INVOKABLE void resetPanes();
    QList<int> paneRotations() const;
    static bool validRotation(int seconds) { return seconds == 0 || seconds == 5 || seconds == 10 || seconds == 30 || seconds == 60; }
    Q_INVOKABLE QString setPaneRotation(int slot, int seconds);
    QVariantList paneRotationSets() const;
    // Add the widget to the slot's rotation set, or remove it if present. An empty set means all widgets.
    Q_INVOKABLE QString togglePaneRotationMember(int slot, const QString& widget_id);
    Q_INVOKABLE QString clearPaneRotationSet(int slot);

    void setAppearance(const QString& appearance);
    // Validate and save; returns an empty string or a user-facing error.
    Q_INVOKABLE QString saveStation(const QString& callsign, double de_lat, double de_lon,
                                    double dx_lat, double dx_lon, const QString& de_zone);
    Q_INVOKABLE QString saveCluster(const QString& host, int port);
    // SSID 0-99 (0 = none); commands one per line, filtered to receive-only commands.
    Q_INVOKABLE QString saveClusterLogin(int ssid, const QString& commands);
    Q_INVOKABLE QString saveWatchList(const QString& entries); // space/comma separated
    Q_INVOKABLE QString saveAdifPath(const QString& path);
    // program: POTA, SOTA or empty; band like 20m; mode letters/digits; age 0-1440 min (0 = any).
    Q_INVOKABLE QString saveOtaSort(const QString& sort);
    Q_INVOKABLE QString saveOtaFilter(const QString& program, const QString& band, const QString& mode, int max_age_min);
    // host letters/digits/dots/hyphens (empty = rotate.aprs2.net); port 1-65535; radius 1-500 km.
    Q_INVOKABLE QString saveAprs(const QString& host, int port, int radius_km);
    // PSKReporter display limit: only receivers within max_km of DE (0-20000, 0 = any).
    Q_INVOKABLE QString savePskMaxKm(int max_km);
    // Receiver call/grid prefixes to keep ("VK, JA, PM95"); empty = all. Max 8 tokens.
    Q_INVOKABLE QString savePskMatch(const QString& filter);
    Q_INVOKABLE QString saveImperialUnits(bool imperial);
    Q_INVOKABLE QString saveClockFormat(int date_order, bool h12);
    Q_INVOKABLE QString saveWeekStart(bool sunday);
    Q_INVOKABLE QString saveBannerStyle(int style);
    Q_INVOKABLE QString saveBannerBackground(int style);
    Q_INVOKABLE QString saveBannerTitle(const QString& title);
    // TV overscan inset: 0..10 % of width/height kept free on every edge.
    Q_INVOKABLE QString saveOverscan(int percent);
    Q_INVOKABLE QString saveUdpSpots(int port, bool lan);
    Q_INVOKABLE QString saveUdpScope(bool by_me);
    // Per-source map on/off for spot layers (keys dx psk ota hamalert aprs adif); default on.
    Q_INVOKABLE bool mapSourceVisible(const QString& key) const { return !map_hidden_.contains(key); }
    Q_INVOKABLE QString saveMapSourceVisible(const QString& key, bool visible);
    // Empty password keeps the stored one; service: "none", "qrz" or "hamqth".
    Q_INVOKABLE QString saveAccount(const QString& service, const QString& user, const QString& password);
    Q_INVOKABLE void clearAccount();
    // HamAlert.org telnet login; empty password keeps the stored one, empty user clears both.
    Q_INVOKABLE QString saveHamAlert(const QString& user, const QString& password);

signals:
    void stationChanged();
    void aprsChanged(); // only when the APRS-IS server, port or radius actually changed
    void appearanceChanged();
    void optionsChanged();
    void accountsChanged();
    void panesChanged();

private:
    void loadOptions();
    void loadAccounts();
    QString writeAccounts();

    QString config_dir_, station_path_, load_error_;
    StationSettings station_{};  // value-init: coordinates are 0, never indeterminate, when no station file exists
    bool configured_ = false;
    QString cluster_host_;
    int cluster_port_ = 7300;
    int cluster_ssid_ = 0;
    QStringList cluster_commands_;
    QStringList watch_list_;
    QString adif_path_;
    QString ota_program_, ota_band_, ota_mode_;
    int ota_max_age_min_ = 0;
    QString ota_sort_;  // "", "freq" or "dist" (empty = newest first)
    QString aprs_host_ = "rotate.aprs2.net";
    int psk_max_km_ = 0;
    QString psk_match_;
    int aprs_port_ = 14580, aprs_radius_km_ = 100;
    int udp_port_ = 0;
    bool udp_lan_ = false;
    bool udp_by_me_ = false;
    QStringList map_hidden_;
    bool imperial_ = false;
    int date_order_ = 0;
    bool clock_12h_ = false;
    bool week_sunday_ = false;
    int banner_style_ = 0;
    int banner_bg_ = 0;
    QString banner_title_;
    int overscan_ = 0;
    QList<int> rotations_;
    QList<QStringList> rotation_sets_;
    QStringList panes_;   // empty entry = default for that slot
    QString lookup_service_ = "none", lookup_user_, lookup_password_;
    QString hamalert_user_, hamalert_password_;
};

} // namespace hamclock
